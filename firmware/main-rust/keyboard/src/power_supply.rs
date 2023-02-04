//! Battery charging and power states.
//!
//! NOTE: The PCB always charges with a current of 0.3A and does not provide the circuitry required
//! for charger detection. It therefore always charges with that current whenever 5V from USB are
//! connected. This design is not compliant to the standard.

use embassy_futures::join::join;
use embassy_futures::select::select;
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};

pub struct BatteryVoltage {
    low: u32,
    high: u32,
}

pub trait PowerSupplyHardware {
    async fn measure_battery_voltage() -> BatteryVoltage;
    fn configure_charging(active: bool);
    fn configure_discharging(low: bool, high: bool);
    fn has_usb_connection() -> bool;
}

pub enum PowerSupplyMode {
    /// The batteries are not low, and we are not charging the batteries.
    Normal,
    /// We are currently charging the batteries.
    Charging,
    /// We are not charging the batteries, and the voltage is low. In this
    /// state, the system should transition into system-off mode.
    Low,
}

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
pub struct PowerSupply<'a> {
    stop: Receiver<'a, NoopRawMutex, (), 1>,
    output: Sender<'a, NoopRawMutex, PowerSupplyState, 1>,
}

impl<'a> PowerSupply<'a> {
    pub fn new(
        stop: Receiver<'a, NoopRawMutex, (), 1>,
        output: Sender<'a, NoopRawMutex, PowerSupplyState, 1>,
    ) -> Self {
        // TODO
        PowerSupply { stop, output }
    }

    // Read the inputs and configure the initial charging state.
    // TODO
    // We send the first state so that the remaining firmware immediately has usable data.
    // TODO

    pub async fn run(&mut self) {
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

    struct ChargingTest {
        low_voltage: u32,
        high_voltage: u32,
        usb_connected: bool,

        should_charge: bool,
        should_discharge_low: bool,
        should_discharge_high: bool,
        expected_mode: PowerSupplyMode,
        expect_callback: bool,
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
                expect_callback: true,
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
                expect_callback: false,
            },
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1050,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Low,
                expect_callback: false,
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
                expect_callback: true,
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
                expect_callback: true,
            },
            // One battery full, USB connected (counts as "charging"
            // as we are balancing the batteries).
            ChargingTest {
                low_voltage: 1100,
                high_voltage: 1400,
                usb_connected: true,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: true,
                expected_mode: PowerSupplyMode::Charging,
                expect_callback: true,
            },
            ChargingTest {
                low_voltage: 1400,
                high_voltage: 1100,
                usb_connected: true,
                should_charge: false,
                should_discharge_low: true,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Charging,
                expect_callback: false,
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
                expect_callback: false,
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
                expect_callback: false,
            },
            ChargingTest {
                low_voltage: 1200,
                high_voltage: 1100,
                usb_connected: true,
                should_charge: true,
                should_discharge_low: true,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Charging,
                expect_callback: false,
            },
            // USB disconnected again - we do not want to discharge
            // if USB is not connected.
            ChargingTest {
                low_voltage: 1200,
                high_voltage: 1100,
                usb_connected: false,
                should_charge: false,
                should_discharge_low: false,
                should_discharge_high: false,
                expected_mode: PowerSupplyMode::Normal,
                expect_callback: true,
            },
        ];

        for test in tests {
            // TODO
        }
    }

    #[futures_test::test]
    async fn test_soc() {
        // TODO
    }
}
