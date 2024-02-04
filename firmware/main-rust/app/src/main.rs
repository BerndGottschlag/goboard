//! Firmware for the "Goboard" custom keyboard (hardware-specific parts).
//!
//! # Structure
//!
//! The firmware is written using the `embassy` framework. For Bluetooth Low Energy, it uses the
//! `softdevice` runtime. This structure means that different code has to be executed at different
//! priorities, so two different embassy executors are used. The main executor runs at a low
//! priority and implements everything except for the Unifying radio protocol. This protocol is
//! implemented using the timeslot API of softdevice so that BLE and Unifying can coexist.
//! Therefore, the Unifying implementation has strict realtime requirements (it needs to complete
//! its tasks before its timeslot expires), so the code is executed in an interrupt executor at a
//! higher priority (see `run_esb()`).
//!
//! This structure impacts both data flow as well as overall control flow. Events are passed to the
//! Unifying code and back using embassy channels and the shutdown/low power logic makes sure that
//! the Unifying code has finished before shutting the system down via Softdevice.
#![no_std]
#![no_main]
#![allow(incomplete_features)]
#![feature(type_alias_impl_trait)]
#![macro_use]

mod key_matrix;
mod power;
//mod usb;

use defmt_rtt as _; // global logger
use embassy_nrf as _; // time driver
use panic_probe as _;

use core::mem;

use cortex_m_rt::entry;
use defmt::{error, info, unwrap};
use embassy_executor::{Executor, InterruptExecutor};
use embassy_nrf::interrupt;
use embassy_nrf::interrupt::InterruptExt;
use embassy_nrf::{pwm, wdt, Peripherals};
use embassy_sync::blocking_mutex::raw::CriticalSectionRawMutex;
use embassy_sync::channel::{Channel, Receiver, Sender};
use nrf_softdevice::{raw, Softdevice};
use static_cell::StaticCell;

use key_matrix::KeyMatrix;
use keyboard::dispatcher::{RadioInEvent, RadioOutEvent};
use keyboard::keys::{Keys, KeysOutEvent};
use keyboard::power_supply::{ChargingHardware, PowerSupply};
use keyboard::Timer;
use power::Battery;

struct EmbassyTimer;

impl Timer for EmbassyTimer {
    async fn after(&self, duration: embassy_time::Duration) {
        embassy_time::Timer::after(duration).await
    }
}

/// Parts of the firmware that interact with ESB (i.e., Unifying protocol implementation).
#[embassy_executor::task]
async fn run_esb(
    input: Receiver<'static, CriticalSectionRawMutex, RadioInEvent, 1>,
    output: Sender<'static, CriticalSectionRawMutex, RadioOutEvent, 1>,
) {
    // TODO: Dummy code
    /*match event {
        RadioInEvent::KeysChanged(key_state) => {}
        RadioInEvent::PowerTransition(power_level) => {
            // TODO
        }
    }*/
    // TODO
}

#[embassy_executor::task]
async fn run_main(
    p: Peripherals,
    unifying_input: Sender<'static, CriticalSectionRawMutex, RadioInEvent, 1>,
    unifying_output: Receiver<'static, CriticalSectionRawMutex, RadioOutEvent, 1>,
) {
    // We want a watchdog timer here that resets the system when there is a panic.
    // TODO: Actually feed the watchdog!
    let mut config = wdt::Config::default();
    config.timeout_ticks = 2 << 16; // The watchdog should be fed once per two seconds.
    config.run_during_debug_halt = false;
    let (_wdt, [_handle]) = match wdt::Watchdog::try_new(p.WDT, config) {
        Ok(x) => x,
        Err(_) => {
            error!("Watchdog already active with wrong config.");
            loop {}
        }
    };

    {
        let input = Channel::new();
        let output = Channel::new();
        let key_matrix =
            KeyMatrix::new(p.P0_22, p.P0_12, p.P0_07, p.SPI3, p.P1_00, p.P0_05, p.P0_04);
        let mut keys = Keys::new(key_matrix, input.receiver(), output.sender()).await;

        //let power_supply_input = Channel::new();
        //let power_supply_output = Channel::new();
        let mut battery = Battery::new(
            p.SAADC, p.P0_28, p.P1_11, p.P1_13, p.P0_30, p.P0_31, p.P0_02,
        );
        let voltages = battery.measure_battery_voltage().await;
        info!("voltages: {}mV/{}mV", voltages.low, voltages.high);
        //let mut power_supply = PowerSupply::new(power_supply_input, power_supply_output, battery, usb_conn).await;

        let keys_function = async {
            keys.run(&EmbassyTimer {}).await;
        };

        // TODO: Experimental code.
        //let seq_words: [u16; 8] = [0, 500, 750, 875, 932, 875, 750, 500];
        let mut seq_words = [1000u16; 128];
        let mut brightness = 1000;
        for i in 0..64 {
            seq_words[i] = 1000 - brightness;
            brightness = brightness * 10 / 11;
        }
        for i in 64..128 {
            seq_words[i] = seq_words[128 - i - 1];
        }

        let mut config = pwm::Config::default();
        config.prescaler = pwm::Prescaler::Div128;
        let mut seq_config = pwm::SequenceConfig::default();
        seq_config.refresh = 4; // 8ms per period => 4*8 = 32ms per step in the sequence.

        let mut pwm = unwrap!(pwm::SequencePwm::new_1ch(p.PWM0, p.P1_03, config,));

        let sequencer = pwm::SingleSequencer::new(&mut pwm, &seq_words, seq_config);
        unwrap!(sequencer.start(pwm::SingleSequenceMode::Infinite));

        let print_function = async {
            loop {
                let event = output.receive().await;
                match event {
                    KeysOutEvent::KeysChanged(keys) => {
                        info!("keys pressed:");
                        for key in keys.into_iter() {
                            info!("  {:?}", key);
                        }
                        // TODO
                    }
                    other => {
                        info!("other key event: {:?}", other);
                    }
                }
            }
        };

        embassy_futures::join::join(keys_function, print_function).await;
    }

    // When we reach this point, the keyboard is supposed to be shut down (either because the
    // batteries are empty or because no connection is selected). In this situation, most of the
    // peripherals have already been reconfigured for low-power operation in the drop() method of
    // the corresponding types, but we still need to perform a number of additional steps to ensure
    // that we are able to transition into a low-power state:
    //
    // 1. We stop the ESB task and wait until it is finished. This step ensures that the radio is
    //    in a known good state when we shut the softdevice down. TODO: Are there even situations
    //    where the ESB task has not been stopped by the dispatcher yet? Can we ensure that
    //    timeouts during communication with the ESB task are caught via a watchdog?
    // 2. We manually configure all GPIOs so that overall power consumption is minimized and
    //    changes to the relevant inputs (mode switch, USB voltage) trigger a reset.
    // 3. Switch to System OFF mode to conserve power.

    // TODO: Stop the ESB task. May not actually be needed, see above.

    // TODO: Configure the GPIOs.

    // Switch to system OFF mode.
    //unsafe { nrf_softdevice_s140::sd_power_system_off() };

    // If for some reason we cannot switch to system OFF mode, we use the watchdog timer to perform
    // a system reset. One exception is debug mode in which case we do not want the watchdog to
    // fire but instead want to block the whole system here so that the debugger remains attached.

    /*let mut config = wdt::Config::default();
    config.timeout_ticks = 1; // The actual timeout will be slightly higher.
    config.run_during_debug_halt = false;
    let (_wdt, [_handle]) = match wdt::Watchdog::try_new(p.WDT, config) {
        Ok(x) => x,
        Err(_) => {
            error!("Watchdog already active with wrong config.");
            loop {}
        }
    };*/
    loop {}
}

static ESB_EXECUTOR: InterruptExecutor = InterruptExecutor::new();
static NORMAL_EXECUTOR: StaticCell<Executor> = StaticCell::new();

#[interrupt]
unsafe fn SWI1_EGU1() {
    ESB_EXECUTOR.on_interrupt()
}

#[entry]
fn main() -> ! {
    info!("Starting goboard main firmware...");

    // Embassy must use the correct interrupt priorities (taken from the nrf-softdevice README.md).
    let mut config = embassy_nrf::config::Config::default();
    config.gpiote_interrupt_priority = interrupt::Priority::P2;
    config.time_interrupt_priority = interrupt::Priority::P2;
    let peripherals = embassy_nrf::init(config);

    // We need to configure the softdevice even if we do not use bluetooth for the currently
    // selected mode.
    let config = nrf_softdevice::Config {
        clock: Some(raw::nrf_clock_lf_cfg_t {
            source: raw::NRF_CLOCK_LF_SRC_RC as u8,
            rc_ctiv: 4,
            rc_temp_ctiv: 2,
            accuracy: 7,
        }),
        conn_gap: Some(raw::ble_gap_conn_cfg_t {
            conn_count: 6,
            event_length: 24,
        }),
        conn_gatt: Some(raw::ble_gatt_conn_cfg_t { att_mtu: 256 }),
        gatts_attr_tab_size: Some(raw::ble_gatts_cfg_attr_tab_size_t {
            attr_tab_size: 32768,
        }),
        gap_role_count: Some(raw::ble_gap_cfg_role_count_t {
            adv_set_count: 1,
            periph_role_count: 3,
            central_role_count: 3,
            central_sec_count: 0,
            _bitfield_1: raw::ble_gap_cfg_role_count_t::new_bitfield_1(0),
        }),
        gap_device_name: Some(raw::ble_gap_cfg_device_name_t {
            p_value: b"RustKbd" as *const u8 as _,
            current_len: 7,
            max_len: 7,
            write_perm: unsafe { mem::zeroed() },
            _bitfield_1: raw::ble_gap_cfg_device_name_t::new_bitfield_1(
                raw::BLE_GATTS_VLOC_STACK as u8,
            ),
        }),
        ..Default::default()
    };
    let _sd = Softdevice::enable(&config);

    static UNIFYING_INPUT: Channel<CriticalSectionRawMutex, RadioInEvent, 1> = Channel::new();
    static UNIFYING_OUTPUT: Channel<CriticalSectionRawMutex, RadioOutEvent, 1> = Channel::new();

    // As softdevice is always active, ESB RF code (e.g., Unifying) needs to use the timeslot API.
    // As that API has strict realtime requirements, we run it in a separate executor with a higher
    // priority. The softdevice uses IRQ priorities 0, 1, and 4, so we run this code at priority 5.
    interrupt::SWI1_EGU1.set_priority(interrupt::Priority::P5);
    let spawner = ESB_EXECUTOR.start(interrupt::SWI1_EGU1);
    unwrap!(spawner.spawn(run_esb(UNIFYING_INPUT.receiver(), UNIFYING_OUTPUT.sender())));

    // Everything else is executed in thread mode.
    let executor = NORMAL_EXECUTOR.init(Executor::new());
    executor.run(|spawner| {
        unwrap!(spawner.spawn(run_main(
            peripherals,
            UNIFYING_INPUT.sender(),
            UNIFYING_OUTPUT.receiver()
        )));
    });

    // The firmware is structured as a pipeline:
    // - The key matrix module scans the key matrix, performs debouncing, and outputs its state
    //   into a channel.
    // - The next pipeline step scans the key state for special keys that reconfigure the keyboard
    //   itself (e.g., switching between Bluetooth and Unifying, re-pairing, etc.).
    // - This module then selects the corresponding transport (USB, BLE, Unifying) and forwards the
    //   filtered key state.
    // - TODO: Where are LEDs configured?

    /*let server = unwrap!(Server::new(sd));
    unwrap!(spawner.spawn(softdevice_task(sd)));

    #[rustfmt::skip]
    let adv_data = &[
        0x02, 0x01, raw::BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE as u8,
        0x03, 0x03, 0x12, 0x18,
        0x03, 0x19, 0xc1, 0x03,
        0x08, 0x09, b'R', b'u', b's', b't', b'K', b'B', b'D',
    ];
    #[rustfmt::skip]
    let scan_data = &[
        0x03, 0x03, 0x12, 0x18,
    ];

    static BONDER: StaticCell<Bonder> = StaticCell::new();
    let bonder = BONDER.init(Bonder::default());

    loop {
        let config = peripheral::Config::default();
        let adv = peripheral::ConnectableAdvertisement::ScannableUndirected {
            adv_data,
            scan_data,
        };
        let conn = unwrap!(peripheral::advertise_pairable(sd, adv, &config, bonder).await);

        info!("advertising done!");

        // Run the GATT server on the connection. This returns when the connection gets disconnected.
        let res = gatt_server::run(&conn, &server, |_| {}).await;

        if let Err(e) = res {
            info!("gatt_server run exited with error: {:?}", e);
        }
    }*/
}
