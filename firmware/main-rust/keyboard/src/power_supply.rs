//! Battery charging and power states.
//!
//! NOTE: The PCB always charges with a current of 0.3A and does not provide the circuitry required
//! for charger detection. It therefore always charges with that current whenever 5V from USB are
//! connected. This design is not compliant to the standard.

use embassy_futures::join::join;
use embassy_futures::select::select;
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};
use embassy_sync::mutex::Mutex;
use embassy_time::Duration;

use super::{debug, Timer};

const CHARGING_PERIOD: Duration = Duration::from_secs(10);
const RECOVERY_PERIOD: Duration = Duration::from_secs(3);

const CHARGE_END_VOLTAGE: u32 = 1380;
const DISCHARGED_VOLTAGE: u32 = 1100;

#[derive(Clone, Copy)]
pub struct BatteryVoltage {
    pub low: u32,
    pub high: u32,
}

#[allow(async_fn_in_trait)]
pub trait ChargingHardware {
    async fn measure_battery_voltage(&mut self) -> BatteryVoltage;
    fn configure_charging(&mut self, active: bool);
    fn configure_discharging(&mut self, low: bool, high: bool);
}

#[allow(async_fn_in_trait)]
pub trait UsbConnection {
    async fn wait_for_change(&mut self);
    fn connected(&mut self) -> bool;
}

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum PowerSupplyMode {
    /// The batteries are not low, and we are not charging the batteries.
    Normal,
    /// We are currently charging the batteries.
    Charging,
    /// We are not charging the batteries, and the voltage is low. In this
    /// state, the system should transition into system-off mode.
    Low,
}

#[derive(Clone, Copy, PartialEq, Debug)]
pub struct PowerSupplyState {
    pub mode: PowerSupplyMode,
    pub battery_charge: u8,
    pub usb_connected: bool,
}

/// Power supply and battery management.
///
/// The power supply code performs a number of tasks:
///
/// - It periodically measures the battery voltages. The lower voltage is
///   used to calculate the remaining.
/// - If USB is connected, the code charges the batteries inbetween voltage
///   measurements if both batteries are below a safe maximum voltage. After
///   each charging period, the code waits for some time without charging to let
///   the battery voltage normalize before the following voltage measurement.
/// - If USB is connected and the battery voltages are substantially different,
///   the code actively discharges the battery with the higher voltage to
///   balance the charge.
pub struct PowerSupply<'a, ChargingHw: ChargingHardware, UsbConn: UsbConnection> {
    stop: Receiver<'a, NoopRawMutex, (), 1>,
    output: Sender<'a, NoopRawMutex, PowerSupplyState, 1>,
    charging_hw: ChargingHw,
    usb_conn: UsbConn,
    state: Mutex<NoopRawMutex, PowerSupplyState>,
}

impl<'a, ChargingHw: ChargingHardware, UsbConn: UsbConnection>
    PowerSupply<'a, ChargingHw, UsbConn>
{
    pub async fn new(
        stop: Receiver<'a, NoopRawMutex, (), 1>,
        output: Sender<'a, NoopRawMutex, PowerSupplyState, 1>,
        mut charging_hw: ChargingHw,
        mut usb_conn: UsbConn,
    ) -> PowerSupply<'a, ChargingHw, UsbConn> {
        // Read the inputs and configure the initial charging state.
        let usb_connected = usb_conn.connected();
        let initial_state = Self::configure_charging(&mut charging_hw, usb_connected).await;

        // We send the first state so that the remaining firmware immediately has usable data.
        output.send(initial_state).await;

        PowerSupply {
            stop,
            output,
            charging_hw,
            usb_conn,
            state: Mutex::new(initial_state),
        }
    }

    pub async fn run<Sleep: Timer>(&mut self, sleep: &Sleep) {
        // We implement listening for USB connection changes outside of the main power supply loop
        // so that state changes are quickly (faster than the charge delays) propagated to the rest
        // of the firmware.
        let usb_connection_function = async {
            loop {
                self.usb_conn.wait_for_change().await;
                // TODO: Implement debouncing?

                // The charging function receives the information via the state variable. We also
                // want to send an event if the state changed so that the rest of the firmware is
                // immediately notified.
                let mut state = self.state.lock().await;
                let prev_connected = state.usb_connected;
                state.usb_connected = self.usb_conn.connected();
                if prev_connected != state.usb_connected {
                    self.output.send(*state).await;
                }
            }
        };

        let charging_function = async {
            loop {
                // We start with the charging period because the constructor already configured the
                // charging hardware.
                sleep.after(CHARGING_PERIOD).await;

                self.charging_hw.configure_discharging(false, false);
                self.charging_hw.configure_charging(false);

                sleep.after(RECOVERY_PERIOD).await;

                let mut state = self.state.lock().await;
                let new_state =
                    Self::configure_charging(&mut self.charging_hw, state.usb_connected).await;
                if *state != new_state {
                    *state = new_state;
                    self.output.send(*state).await;
                }
            }
        };

        select(
            join(usb_connection_function, charging_function),
            self.stop.receive(),
        )
        .await;
    }

    async fn configure_charging(
        charging_hw: &mut ChargingHw,
        usb_connected: bool,
    ) -> PowerSupplyState {
        let voltage = charging_hw.measure_battery_voltage().await;
        debug!("battery voltage: {}mV, {}mV", voltage.low, voltage.high);
        debug!("usb: {}", usb_connected);

        // We derive the state of charge from the battery with the lower voltage. It does not
        // matter if the other battery would be able to provide more charge as the batteries are
        // connected in series.
        // TODO

        let mut mode = PowerSupplyMode::Normal;
        if usb_connected {
            if voltage.low < CHARGE_END_VOLTAGE && voltage.high < CHARGE_END_VOLTAGE {
                debug!("charging.");
                charging_hw.configure_charging(true);
                mode = PowerSupplyMode::Charging;
            }
            // Balancing counts as "charging", even if one battery is
            // already full (the other will be charged when possible).
            let difference = voltage.high as i32 - voltage.low as i32;
            if difference > 20 {
                debug!("discharging 1.");
                charging_hw.configure_discharging(false, true);
                mode = PowerSupplyMode::Charging;
            } else if difference < -20 {
                debug!("discharging 2.");
                charging_hw.configure_discharging(true, false);
                mode = PowerSupplyMode::Charging;
            }
        } else {
            if voltage.low < DISCHARGED_VOLTAGE || voltage.high < DISCHARGED_VOLTAGE {
                mode = PowerSupplyMode::Low;
            }
        }
        PowerSupplyState {
            mode,
            battery_charge: 0, // TODO
            usb_connected,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::MockTimer;

    use embassy_sync::channel::Channel;
    use embassy_time::with_timeout;

    use std::sync::atomic::{AtomicBool, Ordering};
    use std::sync::Mutex;

    struct MockChargingHardware {
        data: Mutex<MockChargingHardwareData>,
    }

    impl MockChargingHardware {
        fn new() -> Self {
            MockChargingHardware {
                data: Mutex::new(MockChargingHardwareData {
                    voltage: BatteryVoltage { low: 0, high: 0 },
                    charging: false,
                    discharging_low: false,
                    discharging_high: false,
                }),
            }
        }
    }

    impl ChargingHardware for &MockChargingHardware {
        async fn measure_battery_voltage(&mut self) -> BatteryVoltage {
            self.data.lock().unwrap().voltage
        }

        fn configure_charging(&mut self, active: bool) {
            self.data.lock().unwrap().charging = active;
        }

        fn configure_discharging(&mut self, low: bool, high: bool) {
            let mut data = self.data.lock().unwrap();
            data.discharging_low = low;
            data.discharging_high = high;
        }
    }

    struct MockChargingHardwareData {
        voltage: BatteryVoltage,
        charging: bool,
        discharging_low: bool,
        discharging_high: bool,
    }

    struct MockUsbConnection {
        connected: AtomicBool,
        change: Channel<NoopRawMutex, (), 1>,
    }

    impl MockUsbConnection {
        fn new(connected: bool) -> Self {
            MockUsbConnection {
                connected: AtomicBool::new(connected),
                change: Channel::new(),
            }
        }

        fn set_connected(&self, connected: bool) {
            self.connected.store(connected, Ordering::SeqCst);
        }

        fn notify(&self) {
            self.change.try_send(()).ok();
        }
    }

    impl UsbConnection for &MockUsbConnection {
        async fn wait_for_change(&mut self) {
            self.change.recv().await;
        }

        fn connected(&mut self) -> bool {
            self.connected.load(Ordering::SeqCst)
        }
    }

    #[derive(Debug)]
    struct ChargingTest {
        low_voltage: u32,
        high_voltage: u32,
        usb_connected: bool,

        should_charge: bool,
        should_discharge_low: bool,
        should_discharge_high: bool,
        expected_mode: PowerSupplyMode,
    }

    impl ChargingTest {
        async fn verify(
            &self,
            output: &Channel<NoopRawMutex, PowerSupplyState, 1>,
            hw: &MockChargingHardware,
            prev_state: PowerSupplyState,
        ) -> PowerSupplyState {
            // We only expect an event if the power supply state changed.
            let expected_state = PowerSupplyState {
                mode: self.expected_mode,
                battery_charge: 0, // TODO
                usb_connected: self.usb_connected,
            };
            let state = if expected_state != prev_state {
                let state = with_timeout(Duration::from_secs(1), output.recv())
                    .await
                    .expect(&std::format!(
                        "waiting for state update failed, test: {:?}",
                        self
                    ));
                assert_eq!(state, expected_state);
                state
            } else {
                prev_state
            };

            let hw_data = hw.data.lock().unwrap();
            assert_eq!(hw_data.charging, self.should_charge, "charging wrong");
            assert_eq!(
                hw_data.discharging_low, self.should_discharge_low,
                "discharging_low wrong"
            );
            assert_eq!(
                hw_data.discharging_high, self.should_discharge_high,
                "discharging_high wrong"
            );

            state
        }
    }

    #[futures_test::test]
    async fn test_charging() {
        env_logger::init();

        let tests = [
            // Both batteries low.
            ChargingTest {
                low_voltage: 1050,
                high_voltage: 1050,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Low,
            },
            // One battery low.
            ChargingTest {
                low_voltage: 1050,
                high_voltage: 1400,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Low,
            },
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1050,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Low,
            },
            // Both batteries full.
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1400,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Normal,
            },
            // Both batteries full, USB connected.
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1400,
                usb_connected: true,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Normal,
            },
            // One battery full, USB connected (counts as "charging" as we are balancing the
            // batteries).
            ChargingTest {
                low_voltage: 1100,
                high_voltage: 1400,
                usb_connected: true,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: true,
                expected_mode: PowerSupplyMode::Charging,
            },
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1100,
                usb_connected: true,
                should_charge: false,
                should_discharge_low: true,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Charging,
            },
            // USB connected, charging without balancing.
            ChargingTest {
                low_voltage: 1100,
                high_voltage: 1100,
                usb_connected: true,
                should_charge: true,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Charging,
            },
            // USB connected, charging with balancing.
            ChargingTest {
                low_voltage: 1100,
                high_voltage: 1200,
                usb_connected: true,
                should_charge: true,
                should_discharge_low: false,
                should_discharge_high: true,
                expected_mode: PowerSupplyMode::Charging,
            },
            ChargingTest {
                low_voltage: 1200,
                high_voltage: 1100,
                usb_connected: true,
                should_charge: true,
                should_discharge_low: true,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Charging,
            },
            // USB disconnected again - we do not want to discharge if USB is not connected.
            ChargingTest {
                low_voltage: 1200,
                high_voltage: 1100,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Normal,
            },
        ];

        let timer = MockTimer::new();
        let usb_connection = MockUsbConnection::new(false);
        let charging_hardware = MockChargingHardware::new();
        let mut state = PowerSupplyState {
            mode: PowerSupplyMode::Normal,
            battery_charge: 42,
            usb_connected: false, // Needs to match the first test
        };

        // We configure the first test and immediately read a result as the initial iteration is
        // executed directly in the constructor.
        usb_connection.set_connected(tests[0].usb_connected);
        {
            let mut charging_data = charging_hardware.data.lock().unwrap();
            charging_data.voltage.low = tests[0].low_voltage;
            charging_data.voltage.high = tests[0].high_voltage;
        }

        let stop = Channel::new();
        let output = Channel::new();
        let mut ps = PowerSupply::new(
            stop.receiver(),
            output.sender(),
            &charging_hardware,
            &usb_connection,
        )
        .await;

        let ps_function = async {
            ps.run(&timer).await;
        };

        let test_function = async {
            for i in 0..tests.len() {
                println!("Test {}: {:?}", i, tests[i]);
                // From this point on, we first expect a charging period, then a recovery period, then a
                // charging period again, ...

                // Whenever the USB connection state changes, we get a second event before the
                // charging period starts.
                if i != 0 && tests[i - 1].usb_connected != tests[i].usb_connected {
                    state.usb_connected = tests[i].usb_connected;
                    let new_state = with_timeout(Duration::from_secs(1), output.recv())
                        .await
                        .expect(&std::format!(
                            "waiting for USB update failed during test {}",
                            i
                        ));
                    assert_eq!(state, new_state);
                }

                with_timeout(Duration::from_secs(1), timer.call_start.recv())
                    .await
                    .expect("charging period timer was not called, is run() stuck?");
                state = tests[i].verify(&output, &charging_hardware, state).await;
                timer.expected_durations.send(CHARGING_PERIOD).await;
                with_timeout(Duration::from_secs(1), timer.call_start.recv())
                    .await
                    .expect("recovery period timer was not called, is run() stuck?");
                {
                    // In the recovery period, we must not charge the battery.
                    let hw_data = charging_hardware.data.lock().unwrap();
                    assert_eq!(hw_data.charging, false);
                    assert_eq!(hw_data.discharging_low, false);
                    assert_eq!(hw_data.discharging_high, false);
                }
                if i == tests.len() - 1 {
                    stop.send(()).await;
                } else {
                    usb_connection.set_connected(tests[i + 1].usb_connected);
                    usb_connection.notify();
                    let mut charging_data = charging_hardware.data.lock().unwrap();
                    charging_data.voltage.low = tests[i + 1].low_voltage;
                    charging_data.voltage.high = tests[i + 1].high_voltage;
                    timer.expected_durations.send(RECOVERY_PERIOD).await;
                }

                if i == tests.len() - 1 {
                    break;
                }
            }
        };

        with_timeout(
            Duration::from_secs(5),
            futures::future::join(ps_function, test_function),
        )
        .await
        .expect("run() did not stop");
    }

    #[futures_test::test]
    async fn test_soc() {
        // TODO
    }
}
