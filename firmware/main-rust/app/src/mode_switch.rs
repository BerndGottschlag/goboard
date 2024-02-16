use embassy_nrf::gpio::{Input, Pull};
use embassy_nrf::peripherals::{P0_09, P0_10};

use keyboard::mode_switch::{ModeSwitchPins, SwitchPosition};

pub struct ModeSwitchHardware {
    sw_1: Input<'static, P0_09>,
    sw_2: Input<'static, P0_10>,
    sw_1_current: bool,
    sw_2_current: bool,
}

fn input_to_position(sw_1: bool, sw_2: bool) -> SwitchPosition {
    if sw_2 {
        SwitchPosition::Profile2
    } else if sw_1 {
        SwitchPosition::Profile1
    } else {
        SwitchPosition::OffUsb
    }
}

impl ModeSwitchHardware {
    pub fn new(sw_1: P0_09, sw_2: P0_10) -> ModeSwitchHardware {
        let sw_1 = Input::new(sw_1, Pull::None);
        let sw_2 = Input::new(sw_2, Pull::None);

        let sw_1_current = sw_1.is_high();
        let sw_2_current = sw_2.is_high();
        ModeSwitchHardware {
            sw_1,
            sw_2,
            sw_1_current,
            sw_2_current,
        }
    }
}

impl ModeSwitchPins for ModeSwitchHardware {
    async fn wait_for_change(&mut self) {
        let wait1 = async {
            if self.sw_1_current {
                self.sw_1.wait_for_low().await;
            } else {
                self.sw_1.wait_for_high().await;
            }
        };
        let wait2 = async {
            if self.sw_2_current {
                self.sw_2.wait_for_low().await;
            } else {
                self.sw_2.wait_for_high().await;
            }
        };
        // We invert the "assumed state" exactly when we detect an edge *based on that assumed
        // state*, because that means that we will never miss an edge (unless position() is called,
        // in which case the user of the type knows the actual state and past edges are
        // irrelevant).
        match embassy_futures::select::select(wait1, wait2).await {
            embassy_futures::select::Either::First(_) => self.sw_1_current = !self.sw_1_current,
            embassy_futures::select::Either::Second(_) => self.sw_2_current = !self.sw_2_current,
        }
    }

    fn position(&mut self) -> SwitchPosition {
        self.sw_1_current = self.sw_1.is_high();
        self.sw_2_current = self.sw_2.is_high();
        input_to_position(self.sw_1_current, self.sw_2_current)
    }
}
