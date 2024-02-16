use core::sync::atomic::{AtomicBool, Ordering};

use defmt::*;
use embassy_futures::join::join;
use embassy_futures::select::{select, Either};
use embassy_nrf::peripherals::USBD;
use embassy_nrf::usb::vbus_detect::HardwareVbusDetect;
use embassy_nrf::usb::{self, Driver};
use embassy_nrf::{bind_interrupts, peripherals};
use embassy_sync::blocking_mutex::raw::CriticalSectionRawMutex;
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};
use embassy_sync::signal::Signal;
use embassy_time::Timer;
use embassy_usb::class::hid::{HidReaderWriter, ReportId, RequestHandler, State};
use embassy_usb::control::OutResponse;
use embassy_usb::{Builder, Config, Handler};
use usbd_hid::descriptor::{KeyboardReport, SerializedDescriptor};

use keyboard::dispatcher::{UsbInEvent, UsbOutEvent};
use keyboard::{KeyState, PowerLevel};

bind_interrupts!(struct Irqs {
    USBD => usb::InterruptHandler<peripherals::USBD>;
});

pub struct Usb<'a> {
    usb_in: Receiver<'a, NoopRawMutex, UsbInEvent, 1>,
    usb_out: Sender<'a, NoopRawMutex, UsbOutEvent, 1>,
}

impl<'a> Usb<'a> {
    pub fn new(
        usb_in: Receiver<'a, NoopRawMutex, UsbInEvent, 1>,
        usb_out: Sender<'a, NoopRawMutex, UsbOutEvent, 1>,
    ) -> Self {
        // TODO
        Usb { usb_in, usb_out }
    }

    pub async fn run(&mut self, usbd: USBD, vbus_detect: &usb::vbus_detect::SoftwareVbusDetect) {
        // Create the driver, from the HAL.
        let driver = Driver::new(usbd, Irqs, vbus_detect);

        // Create embassy-usb Config
        let mut config = Config::new(0x1209, 0x000A);
        config.manufacturer = Some("InstinctiveLlama");
        config.product = Some("Goboard");
        config.serial_number = Some("12345678");
        config.max_power = 350;
        config.max_packet_size_0 = 64;
        config.supports_remote_wakeup = true;

        // Create embassy-usb DeviceBuilder using the driver and config.
        // It needs some buffers for building the descriptors.
        let mut device_descriptor = [0; 256];
        let mut config_descriptor = [0; 256];
        let mut bos_descriptor = [0; 256];
        let mut msos_descriptor = [0; 256];
        let mut control_buf = [0; 64];
        let request_handler = MyRequestHandler {};
        let mut device_handler = MyDeviceStateHandler::new();

        let mut state = State::new();

        let mut builder = Builder::new(
            driver,
            config,
            &mut device_descriptor,
            &mut config_descriptor,
            &mut bos_descriptor,
            &mut msos_descriptor,
            &mut control_buf,
        );

        builder.handler(&mut device_handler);

        // Create classes on the builder.
        let config = embassy_usb::class::hid::Config {
            report_descriptor: KeyboardReport::desc(),
            request_handler: Some(&request_handler),
            poll_ms: 60,
            max_packet_size: 64,
        };
        let hid = HidReaderWriter::<_, 1, 8>::new(&mut builder, &mut state, config);

        // Build the builder.
        let mut usb = builder.build();

        let remote_wakeup: Signal<CriticalSectionRawMutex, _> = Signal::new();

        // TODO
        // Run the USB device.
        let usb_fut = async {
            loop {
                usb.run_until_suspend().await;
                match select(usb.wait_resume(), remote_wakeup.wait()).await {
                    Either::First(_) => (),
                    Either::Second(_) => unwrap!(usb.remote_wakeup().await),
                }
            }
        };

        let (reader, mut writer) = hid.split();

        // Do stuff with the class!
        let in_fut = async {
            loop {
                Timer::after_secs(1).await;
                //button.wait_for_low().await;
                info!("PRESSED");

                if SUSPENDED.load(Ordering::Acquire) {
                    info!("Triggering remote wakeup");
                    remote_wakeup.signal(());
                } else {
                    let report = KeyboardReport {
                        keycodes: [4, 0, 0, 0, 0, 0],
                        leds: 0,
                        modifier: 0,
                        reserved: 0,
                    };
                    match writer.write_serialize(&report).await {
                        Ok(()) => {}
                        Err(e) => warn!("Failed to send report: {:?}", e),
                    };
                }

                //button.wait_for_high().await;
                Timer::after_secs(1).await;
                info!("RELEASED");
                let report = KeyboardReport {
                    keycodes: [0, 0, 0, 0, 0, 0],
                    leds: 0,
                    modifier: 0,
                    reserved: 0,
                };
                match writer.write_serialize(&report).await {
                    Ok(()) => {}
                    Err(e) => warn!("Failed to send report: {:?}", e),
                };
            }
        };

        let out_fut = async {
            reader.run(false, &request_handler).await;
        };

        // Run everything concurrently.
        // If we had made everything `'static` above instead, we could do this using separate tasks instead.
        join(usb_fut, join(in_fut, out_fut)).await;
    }
}

static SUSPENDED: AtomicBool = AtomicBool::new(false);

struct MyRequestHandler {}

impl RequestHandler for MyRequestHandler {
    fn get_report(&self, id: ReportId, _buf: &mut [u8]) -> Option<usize> {
        info!("Get report for {:?}", id);
        None
    }

    fn set_report(&self, id: ReportId, data: &[u8]) -> OutResponse {
        info!("Set report for {:?}: {=[u8]}", id, data);
        OutResponse::Accepted
    }

    fn set_idle_ms(&self, id: Option<ReportId>, dur: u32) {
        info!("Set idle rate for {:?} to {:?}", id, dur);
    }

    fn get_idle_ms(&self, id: Option<ReportId>) -> Option<u32> {
        info!("Get idle rate for {:?}", id);
        None
    }
}

struct MyDeviceStateHandler {
    configured: AtomicBool,
}

impl MyDeviceStateHandler {
    fn new() -> Self {
        MyDeviceStateHandler {
            configured: AtomicBool::new(false),
        }
    }
}

impl embassy_usb::Handler for MyDeviceStateHandler {
    fn enabled(&mut self, enabled: bool) {
        self.configured.store(false, Ordering::Relaxed);
        SUSPENDED.store(false, Ordering::Release);
        if enabled {
            info!("Device enabled");
        } else {
            info!("Device disabled");
        }
    }

    fn reset(&mut self) {
        self.configured.store(false, Ordering::Relaxed);
        info!("Bus reset, the Vbus current limit is 100mA");
    }

    fn addressed(&mut self, addr: u8) {
        self.configured.store(false, Ordering::Relaxed);
        info!("USB address set to: {}", addr);
    }

    fn configured(&mut self, configured: bool) {
        self.configured.store(configured, Ordering::Relaxed);
        if configured {
            info!(
                "Device configured, it may now draw up to the configured current limit from Vbus."
            )
        } else {
            info!("Device is no longer configured, the Vbus current limit is 100mA.");
        }
    }

    fn suspended(&mut self, suspended: bool) {
        if suspended {
            info!("Device suspended, the Vbus current limit is 500µA (or 2.5mA for high-power devices with remote wakeup enabled).");
            SUSPENDED.store(true, Ordering::Release);
        } else {
            SUSPENDED.store(false, Ordering::Release);
            if self.configured.load(Ordering::Relaxed) {
                info!(
                    "Device resumed, it may now draw up to the configured current limit from Vbus"
                );
            } else {
                info!("Device resumed, the Vbus current limit is 100mA");
            }
        }
    }
}
