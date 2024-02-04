use defmt::{error, info, unwrap};

use embassy_nrf::bind_interrupts;
use embassy_nrf::gpio::{Level, Output, OutputDrive};
use embassy_nrf::peripherals::{P0_02, P0_28, P0_30, P0_31, P1_11, P1_13, SAADC};
use embassy_nrf::saadc::{self, ChannelConfig, Config, Saadc};

use keyboard::power_supply::{BatteryVoltage, ChargingHardware};

bind_interrupts!(struct Irqs {
    SAADC => saadc::InterruptHandler;
});

pub struct Battery {
    charge: Output<'static, P0_28>,
    balance_low: Output<'static, P1_11>,
    n_balance_high: Output<'static, P1_13>,
    n_measurement_power: Output<'static, P0_30>,
    adc: Saadc<'static, 2>,
    //measurement_pin: P0_31,
    //mid_voltage_pin: P0_02,
    // TODO
}

impl Battery {
    pub fn new(
        saadc: SAADC,
        charge: P0_28,
        balance_low: P1_11,
        n_balance_high: P1_13,
        n_measurement_power: P0_30,
        measurement_pin: P0_31,
        mid_voltage_pin: P0_02,
    ) -> Battery {
        let charge = Output::new(charge, Level::Low, OutputDrive::Standard);
        let balance_low = Output::new(balance_low, Level::Low, OutputDrive::Standard);
        let n_balance_high = Output::new(n_balance_high, Level::High, OutputDrive::Standard);
        let n_measurement_power =
            Output::new(n_measurement_power, Level::High, OutputDrive::Standard);

        let config = Config::default();
        let vbatt = ChannelConfig::single_ended(measurement_pin);
        let vmid = ChannelConfig::single_ended(mid_voltage_pin);
        let mut adc = Saadc::new(saadc, Irqs, config, [vbatt, vmid]);

        Battery {
            charge,
            balance_low,
            n_balance_high,
            n_measurement_power,
            adc,
        }
    }
}

impl Drop for Battery {
    fn drop(&mut self) {
        // We configure the pins to minimize power consumption. We assume that the pins do not
        // implement Drop so that the state remains intact during shutdown.
        self.charge.set_low();
        self.balance_low.set_low();
        self.n_balance_high.set_high();
        self.n_measurement_power.set_high();
        // The ADC implements its own drop method that disables the peripheral.
    }
}

impl ChargingHardware for Battery {
    async fn measure_battery_voltage(&mut self) -> BatteryVoltage {
        self.n_measurement_power.set_low();
        let mut buf = [0; 2];
        self.adc.sample(&mut buf).await;
        self.n_measurement_power.set_high();

        // Full range of the 12-bit values is six times the 0.6V reference by default.
        let low = buf[1] as u32 * 3600 / 4096;
        let total = buf[0] as u32 * 2 * 3600 / 4096;
        let high = if total > low { total - low } else { 0 };

        BatteryVoltage { low, high }
    }

    fn configure_charging(&mut self, active: bool) {
        if active {
            self.charge.set_high();
        } else {
            self.charge.set_low();
        }
    }

    fn configure_discharging(&mut self, low: bool, high: bool) {
        if low {
            self.balance_low.set_high();
        } else {
            self.balance_low.set_low();
        }
        if high {
            self.n_balance_high.set_low();
        } else {
            self.n_balance_high.set_high();
        }
    }
}
