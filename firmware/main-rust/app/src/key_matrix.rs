use keyboard::keys::KeyMatrixHardware;

use defmt::info;
use embassy_embedded_hal::SetConfig;
use embassy_nrf::gpio::{Level, Output, OutputDrive};
use embassy_nrf::peripherals::{P0_04, P0_05, P0_07, P0_12, P0_22, P1_00, SPI3};
use embassy_nrf::{interrupt, spim, spis};

pub struct KeyMatrix {
    n_power: Output<'static, P0_22>,
    store: Output<'static, P0_12>,
    n_load: Output<'static, P0_07>,
    spi: spim::Spim<'static, SPI3>,
}

impl KeyMatrix {
    pub fn new(
        power_pin: P0_22,
        store_pin: P0_12,
        load_pin: P0_07,
        spi: SPI3,
        sck_pin: P1_00,
        mosi_pin: P0_05,
        miso_pin: P0_04,
    ) -> KeyMatrix {
        // The power and load pins are active-low.
        let n_power = Output::new(power_pin, Level::High, OutputDrive::Standard);
        let store = Output::new(store_pin, Level::Low, OutputDrive::Standard);
        let n_load = Output::new(load_pin, Level::High, OutputDrive::Standard);

        // The shift registers are driven via SPI.
        let mut config = spim::Config::default();
        config.frequency = spim::Frequency::M2;
        let irq = interrupt::take!(SPIM3);
        let spi = spim::Spim::new(spi, irq, sck_pin, miso_pin, mosi_pin, config);

        KeyMatrix {
            n_power,
            store,
            n_load,
            spi,
        }
    }
}

impl Drop for KeyMatrix {
    fn drop(&mut self) {
        // TODO: Additional current consumption because GPIOs are not low?
        // TODO: Do we want to do this at all?
        self.n_load.set_high();
        self.store.set_low();
        self.disable();
    }
}

impl KeyMatrixHardware for KeyMatrix {
    async fn enable(&mut self) {
        self.n_power.set_low();
        // We need to wait for some time to make sure that all the capacitance
        // is charged.
        embassy_time::Timer::after(embassy_time::Duration::from_millis(2)).await;
    }

    fn disable(&mut self) {
        self.n_power.set_high();
    }

    async fn read_row(&mut self, row_bitmap: u8) -> u16 {
        // The row selection lines are active-low.
        let row_bitmap = !row_bitmap;
        // Write the row to the output shift register.
        // The two shift registers use different SPI modes, so we cannot use one transfer to
        // read/write both at the same time.
        let mut config = spim::Config::default();
        config.frequency = spim::Frequency::M2;
        config.mode = spis::Mode {
            polarity: spis::Polarity::IdleHigh,
            phase: spis::Phase::CaptureOnFirstTransition,
        };
        self.spi.set_config(&config);
        let mut rx = [0];
        let tx = [row_bitmap];
        self.spi.transfer(&mut rx, &tx).await.unwrap();

        // Perform serial-to-parallel conversion to select the row.
        self.store.set_high();
        // TODO: Use cortex_m::asm::delay();
        embassy_time::Timer::after(embassy_time::Duration::from_micros(1)).await;
        self.store.set_low();
        embassy_time::Timer::after(embassy_time::Duration::from_micros(1)).await;

        // Perform parallel-to-serial conversion to read one row.
        self.n_load.set_low();
        embassy_time::Timer::after(embassy_time::Duration::from_micros(1)).await;
        self.n_load.set_high();
        embassy_time::Timer::after(embassy_time::Duration::from_micros(1)).await;

        // Read the input shift register.
        let mut config = spim::Config::default();
        config.frequency = spim::Frequency::M2;
        config.mode = spis::Mode {
            polarity: spis::Polarity::IdleHigh,
            phase: spis::Phase::CaptureOnSecondTransition,
        };
        let mut rx = [0, 0];
        let tx = [0, 0];
        self.spi.transfer(&mut rx, &tx).await.unwrap();
        info!("transfer result: {}", rx);
        !rx[0] as u16 | (!rx[1] as u16 >> 16)
    }

    /*async fn transfer(&mut self, out: u8) -> u16 {
        // TODO
        /*uint16_t in = 0;
        struct spi_buf buf = {
            .buf = &in,
            .len = sizeof(in),
        };
        struct spi_buf_set buf_set = {
            .buffers = &buf,
            .count = 1,
        };
        if (spi_read(spi_dev, &READ_SPI_CFG, &buf_set) != 0) {
            throw HardwareError("spi_read failed");
        }
        // The row selection lines are active-low.
        out = ~out << 1;
        buf.buf = &out;
        buf.len = sizeof(out);
        if (spi_write(spi_dev, &WRITE_SPI_CFG, &buf_set) != 0) {
            throw HardwareError("spi_write failed");
        }
        // The keys are active-low.
        in = ~in;
        return (in >> 8) | (in << 8);*/
    }*/
}
