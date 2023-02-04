//! Battery charging and power states.
//!
//! NOTE: The PCB always charges with a current of 0.3A and does not provide the circuitry required
//! for charger detection. It therefore always charges with that current whenever 5V from USB are
//! connected. This design is not compliant to the standard.

use embassy_futures::join::join;
use embassy_futures::select::select;
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};
use embassy_time::Duration;

use super::Timer;

const CHARGING_PERIOD: Duration = Duration::from_secs(10);
const RECOVERY_PERIOD: Duration = Duration::from_secs(3);

#[derive(Clone, Copy)]
pub struct BatteryVoltage {
    low: u32,
    high: u32,
}

pub trait ChargingHardware {
    async fn measure_battery_voltage(&mut self) -> BatteryVoltage;
    fn configure_charging(&mut self, active: bool);
    fn configure_discharging(&mut self, low: bool, high: bool);
}

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
    mode: PowerSupplyMode,
    battery_charge: u8,
    usb_connected: bool,
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
}

impl<'a, ChargingHw: ChargingHardware, UsbConn: UsbConnection>
    PowerSupply<'a, ChargingHw, UsbConn>
{
    pub fn new(
        stop: Receiver<'a, NoopRawMutex, (), 1>,
        output: Sender<'a, NoopRawMutex, PowerSupplyState, 1>,
        charging_hw: ChargingHw,
        usb_conn: UsbConn,
    ) -> Self {
        // Read the inputs and configure the initial charging state.
        // TODO
        // We send the first state so that the remaining firmware immediately has usable data.
        // TODO

        PowerSupply {
            stop,
            output,
            charging_hw,
            usb_conn,
        }
    }

    pub async fn run<Sleep: Timer>(&mut self, sleep: &Sleep) {
        // We implement listening for USB connection changes outside of the main power supply loop
        // so that state changes are quickly (faster than the charge delays) propagated to the rest
        // of the firmware.
        let usb_connection_function = async {
            // TODO
        };

        let charging_function = async {
            // TODO
        };

        select(
            join(usb_connection_function, charging_function),
            self.stop.recv(),
        )
        .await;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::{MockTimer, NopMockTimer};

    use embassy_sync::channel::Channel;

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
        // Read the inputs and configure the initial charging state.
        // TODO
        // We send the first state so that the remaining firmware immediately has usable data.
        // TODO

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
        async fn wait_for_change(&mut self) {}
        fn connected(&mut self) -> bool {
            self.connected.load(Ordering::SeqCst)
        }
    }

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
                battery_charge: 100, // TODO
                usb_connected: self.usb_connected,
            };
            let state = if expected_state != prev_state {
                let state = output.recv().await;
                assert_eq!(state, expected_state);
                state
            } else {
                prev_state
            };

            let hw_data = hw.data.lock().unwrap();
            assert_eq!(hw_data.charging, self.should_charge);
            assert_eq!(hw_data.discharging_low, self.should_discharge_low);
            assert_eq!(hw_data.discharging_high, self.should_discharge_high);

            state
        }
    }

    #[futures_test::test]
    async fn test_charging() {
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
            usb_connected: false,
        };

        // We configure the first test and immediately read a result as the initial iteration is
        // executed directly in the constructor.
        let test = &tests[0];
        usb_connection.set_connected(test.usb_connected);
        let mut charging_data = charging_hardware.data.lock().unwrap();
        charging_data.voltage.low = test.low_voltage;
        charging_data.voltage.high = test.high_voltage;

        let stop = Channel::new();
        let output = Channel::new();
        let mut ps = PowerSupply::new(
            stop.receiver(),
            output.sender(),
            &charging_hardware,
            &usb_connection,
        );

        let ps_function = async {
            ps.run(&timer).await;
        };

        let test_function = async {
            for i in 0..tests.len() {
                // From this point on, we first expect a charging period, then a recovery period, then a
                // charging period again, ...
                timer.call_start.recv().await;
                state = tests[0].verify(&output, &charging_hardware, state).await;
                timer.expected_durations.send(CHARGING_PERIOD).await;
                timer.call_start.recv().await;
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
                    usb_connection.set_connected(test.usb_connected);
                    let mut charging_data = charging_hardware.data.lock().unwrap();
                    charging_data.voltage.low = test.low_voltage;
                    charging_data.voltage.high = test.high_voltage;
                }
                timer.expected_durations.send(RECOVERY_PERIOD).await;

                if i == tests.len() - 1 {
                    break;
                }
            }
        };

        futures::future::join(ps_function, test_function).await;
    }

    #[futures_test::test]
    async fn test_soc() {
        // TODO
    }
}
