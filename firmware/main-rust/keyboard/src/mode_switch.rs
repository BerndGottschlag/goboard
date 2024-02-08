//! Debouncing for the mode switch.
//!
//! The mode switch is bouncy, so after each change we wait for 100ms for the switch state to
//! settle. Only then, the code emits a new switch position via a channel.

use defmt::Format;
use embassy_futures::select::select;
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};
use embassy_time::Duration;

use super::Timer;

#[derive(Clone, Copy, Format, Debug, PartialEq)]
pub enum SwitchPosition {
    OffUsb,
    Profile1,
    Profile2,
}

#[allow(async_fn_in_trait)]
pub trait ModeSwitchPins {
    async fn wait_for_change(&mut self);
    fn position(&mut self) -> SwitchPosition;
}

pub struct ModeSwitch<'a, Pins: ModeSwitchPins> {
    stop: Receiver<'a, NoopRawMutex, (), 1>,
    output: Sender<'a, NoopRawMutex, SwitchPosition, 1>,
    pins: Pins,
}

impl<'a, Pins: ModeSwitchPins> ModeSwitch<'a, Pins> {
    pub fn new(
        pins: Pins,
        stop: Receiver<'a, NoopRawMutex, (), 1>,
        output: Sender<'a, NoopRawMutex, SwitchPosition, 1>,
    ) -> Self {
        ModeSwitch { stop, output, pins }
    }

    pub async fn run<Sleep: Timer>(&mut self, sleep: &Sleep) {
        let switch_function = async {
            let mut position = self.pins.position();
            // We send the initial position, as otherwise there is no way to get this state.
            self.output.send(position).await;
            loop {
                self.pins.wait_for_change().await;
                sleep.after(Duration::from_millis(100)).await;
                let new_position = self.pins.position();
                if new_position != position {
                    position = new_position;
                    self.output.send(position).await;
                }
            }
        };

        select(switch_function, self.stop.receive()).await;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::{MockTimer, NopMockTimer};

    use embassy_sync::channel::Channel;

    use std::sync::Mutex;

    struct MockModeSwitchPins {
        data: Mutex<MockModeSwitchPinsData>,
        change: Channel<NoopRawMutex, (), 1>,
    }

    impl MockModeSwitchPins {
        fn new(position: SwitchPosition) -> Self {
            MockModeSwitchPins {
                data: Mutex::new(MockModeSwitchPinsData { position }),
                change: Channel::new(),
            }
        }

        fn set_position(&self, position: SwitchPosition) {
            let mut data = self.data.lock().unwrap();
            data.position = position;
        }

        fn notify(&self) {
            self.change.try_send(()).ok();
        }
    }

    impl ModeSwitchPins for &MockModeSwitchPins {
        async fn wait_for_change(&mut self) {
            self.change.recv().await;
        }
        fn position(&mut self) -> SwitchPosition {
            let data = self.data.lock().unwrap();
            data.position
        }
    }

    struct MockModeSwitchPinsData {
        position: SwitchPosition,
    }

    #[futures_test::test]
    async fn test_modes() {
        let timer = NopMockTimer;
        let pins = MockModeSwitchPins::new(SwitchPosition::Profile1);

        let stop = Channel::new();
        let output = Channel::new();
        let mut switch = ModeSwitch::new(&pins, stop.receiver(), output.sender());

        let switch_function = async {
            switch.run(&timer).await;
        };

        let test_function = async {
            let event = output.recv().await;
            assert_eq!(
                event,
                SwitchPosition::Profile1,
                "unexpected initial position returned"
            );
            for position in [
                SwitchPosition::OffUsb,
                SwitchPosition::Profile1,
                SwitchPosition::Profile2,
            ] {
                pins.set_position(position);
                pins.notify();
                let event = output.recv().await;
                assert_eq!(event, position, "unexpected position returned");
            }
            stop.send(()).await;
        };

        futures::future::join(switch_function, test_function).await;
    }

    #[futures_test::test]
    async fn test_debouncing() {
        let timer = MockTimer::new();
        let pins = MockModeSwitchPins::new(SwitchPosition::OffUsb);

        let stop = Channel::new();
        let output = Channel::new();
        let mut switch = ModeSwitch::new(&pins, stop.receiver(), output.sender());

        let switch_function = async {
            switch.run(&timer).await;
        };

        let test_function = async {
            let event = output.recv().await;
            assert_eq!(
                event,
                SwitchPosition::OffUsb,
                "unexpected initial position returned"
            );
            // Before the sleep, we set a different position. We expect this position to be
            // ignored, as it may be the result of switch bouncing.
            pins.set_position(SwitchPosition::Profile1);
            pins.notify();
            timer.call_start.recv().await;
            pins.set_position(SwitchPosition::Profile2);
            timer
                .expected_durations
                .send(Duration::from_millis(100))
                .await;
            let event = output.recv().await;
            assert_eq!(
                event,
                SwitchPosition::Profile2,
                "unexpected position returned"
            );
            stop.send(()).await;
        };

        futures::future::join(switch_function, test_function).await;
    }
}
