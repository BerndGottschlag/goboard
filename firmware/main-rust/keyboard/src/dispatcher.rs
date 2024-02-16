#![allow(dead_code)]
#![allow(unused)]
use embassy_sync::blocking_mutex::raw::{CriticalSectionRawMutex, NoopRawMutex, RawMutex};
use embassy_sync::channel::{Receiver, Sender};
use embassy_time::Duration;

use super::keys::{KeysInEvent, KeysOutEvent};
use super::mode_switch::SwitchPosition;
use super::power_supply::{PowerSupplyMode, PowerSupplyState};
use super::select6::select6;
use super::{KeyState, PowerLevel, Timer};

const POWER_SAVING_TIMEOUT: Duration = Duration::from_secs(60);

#[derive(PartialEq, Debug)]
pub enum UsbInEvent {
    KeysChanged(KeyState),
    PowerTransition(PowerLevel),
}

#[derive(PartialEq, Debug)]
pub enum UsbOutEvent {
    PowerTransitionDone(PowerLevel),
    ChargingAllowed(bool),
}

#[derive(PartialEq, Debug)]
pub enum RadioInEvent {
    KeysChanged(KeyState),
    PowerTransition(PowerLevel),
}

#[derive(PartialEq, Debug)]
pub enum RadioOutEvent {
    PowerTransitionDone(PowerLevel),
}

pub struct Dispatcher<'a> {
    keys_in: Sender<'a, NoopRawMutex, KeysInEvent, 1>,
    keys_out: Receiver<'a, NoopRawMutex, KeysOutEvent, 1>,
    mode_switch_stop: Sender<'a, NoopRawMutex, (), 1>,
    mode_switch_out: Receiver<'a, NoopRawMutex, SwitchPosition, 1>,
    power_supply_stop: Sender<'a, NoopRawMutex, (), 1>,
    power_supply_out: Receiver<'a, NoopRawMutex, PowerSupplyState, 1>,
    usb_in: Sender<'a, NoopRawMutex, UsbInEvent, 1>,
    usb_out: Receiver<'a, NoopRawMutex, UsbOutEvent, 1>,
    bluetooth_in: Sender<'a, NoopRawMutex, RadioInEvent, 1>,
    bluetooth_out: Receiver<'a, NoopRawMutex, RadioOutEvent, 1>,
    unifying_in: Sender<'a, CriticalSectionRawMutex, RadioInEvent, 1>,
    unifying_out: Receiver<'a, CriticalSectionRawMutex, RadioOutEvent, 1>,
    // TODO: Radio profiles 1 and 2 - Unifying or bluetooth?
    // TODO: State of special keys to detect changes?
}

impl<'a> Dispatcher<'a> {
    pub fn new(
        keys_in: Sender<'a, NoopRawMutex, KeysInEvent, 1>,
        keys_out: Receiver<'a, NoopRawMutex, KeysOutEvent, 1>,
        mode_switch_stop: Sender<'a, NoopRawMutex, (), 1>,
        mode_switch_out: Receiver<'a, NoopRawMutex, SwitchPosition, 1>,
        power_supply_stop: Sender<'a, NoopRawMutex, (), 1>,
        power_supply_out: Receiver<'a, NoopRawMutex, PowerSupplyState, 1>,
        usb_in: Sender<'a, NoopRawMutex, UsbInEvent, 1>,
        usb_out: Receiver<'a, NoopRawMutex, UsbOutEvent, 1>,
        bluetooth_in: Sender<'a, NoopRawMutex, RadioInEvent, 1>,
        bluetooth_out: Receiver<'a, NoopRawMutex, RadioOutEvent, 1>,
        unifying_in: Sender<'a, CriticalSectionRawMutex, RadioInEvent, 1>,
        unifying_out: Receiver<'a, CriticalSectionRawMutex, RadioOutEvent, 1>,
    ) -> Self {
        // TODO: Initial state?
        Self {
            keys_in,
            keys_out,
            mode_switch_stop,
            mode_switch_out,
            power_supply_stop,
            power_supply_out,
            usb_in,
            usb_out,
            bluetooth_in,
            bluetooth_out,
            unifying_in,
            unifying_out,
        }
    }

    pub async fn run<Sleep: Timer>(&mut self, sleep: &Sleep) {
        // The timeout for low-power modes is processed in a different function because the timeout
        // is not cancel-safe.
        let timeout_func = async || {
            // TODO
        };
        let state_func = async || {
            // TODO
        };
        // TODO
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::scancode::ScanCode;
    use crate::test_utils::MockTimer;
    use crate::KeyState;

    use embassy_sync::channel::Channel;
    use embassy_time::with_timeout;

    use std::vec::Vec;

    async fn usb_expect_key_event(
        output: &Channel<NoopRawMutex, UsbInEvent, 1>,
        keys: &[ScanCode],
    ) {
        let event = with_timeout(Duration::from_secs(1), output.receive())
            .await
            .expect("did not generate key change event");
        if let UsbInEvent::KeysChanged(key_state) = event {
            assert_eq!(
                &key_state.into_iter().collect::<Vec<_>>(),
                keys,
                "wrong keys received"
            );
        } else {
            panic!("wrong event received");
        }
    }

    async fn radio_expect_key_event(
        output: &Channel<NoopRawMutex, RadioInEvent, 1>,
        keys: &[ScanCode],
    ) {
        let event = with_timeout(Duration::from_secs(1), output.receive())
            .await
            .expect("did not generate key change event");
        if let RadioInEvent::KeysChanged(key_state) = event {
            assert_eq!(
                &key_state.into_iter().collect::<Vec<_>>(),
                keys,
                "wrong keys received"
            );
        } else {
            panic!("wrong event received");
        }
    }

    async fn expect_event<EventType: PartialEq + core::fmt::Debug, M: RawMutex>(
        output: &Channel<M, EventType, 1>,
        expected: EventType,
    ) {
        let event = with_timeout(Duration::from_secs(1), output.receive())
            .await
            .expect("did not generate event"); // TODO: Receive with timeout
        assert_eq!(event, expected, "wrong event received");
    }

    #[futures_test::test]
    async fn test_key_forwarding() {
        let timer = MockTimer::new();

        let keys_in = Channel::new();
        let keys_out = Channel::new();
        let mode_switch_in = Channel::new();
        let mode_switch_out = Channel::new();
        let power_supply_in = Channel::new();
        let power_supply_out = Channel::new();
        let usb_in = Channel::new();
        let usb_out = Channel::new();
        let bluetooth_in = Channel::new();
        let bluetooth_out = Channel::new();
        let unifying_in = Channel::new();
        let unifying_out = Channel::new();

        let dispatcher_function = async {
            let mut dispatcher = Dispatcher::new(
                keys_in.sender(),
                keys_out.receiver(),
                mode_switch_in.sender(),
                mode_switch_out.receiver(),
                power_supply_in.sender(),
                power_supply_out.receiver(),
                usb_in.sender(),
                usb_out.receiver(),
                bluetooth_in.sender(),
                bluetooth_out.receiver(),
                unifying_in.sender(),
                unifying_out.receiver(),
            );

            dispatcher.run(&timer).await;
        };

        let test_function = async {
            // Some initial state is required by the dispatcher.
            mode_switch_out.send(SwitchPosition::OffUsb).await;
            power_supply_out.send(PowerSupplyState {
                mode: PowerSupplyMode::Charging,
                battery_charge: 50,
                usb_connected: true,
            });

            // Test that keys are forwarded to the active sink.
            let mut keys = KeyState::new();
            keys.set_key(ScanCode::Escape);
            keys_out.send(KeysOutEvent::KeysChanged(keys)).await;
            usb_expect_key_event(&usb_in, &[ScanCode::Escape]).await;
            // TODO: Other sinks

            // Test that mode switches select a different sink. The previous sink must not send
            // further key presses and all currently pressed keys must be released. The new sink
            // must be notified of the keyboard state.
            mode_switch_out.send(SwitchPosition::Profile1).await;
            usb_expect_key_event(&usb_in, &[]).await;
            // TODO: Notification to bluetooth sink that it became active?
            radio_expect_key_event(&bluetooth_in, &[ScanCode::Escape]).await;
            // TODO: Unifying? We need to mock persistent storage.
            // TODO: Switching between the two profiles? Should we just have one radio sink?

            // Test that power mode changes are forwarded to all sinks.
            // TODO: The following code is wrong. PowerSupplyMode::Low means that the device shuts
            // down!
            //power_supply_out
            //    .send(PowerSupplyState {
            //        mode: PowerSupplyMode::Low,
            //        battery_charge: 10,
            //        usb_connected: false,
            //    })
            //    .await;
            //usb_expect_event(&usb_in, UsbInEvent::PowerTransition(PowerLevel::LowPower)).await;
            //usb_out
            //    .send(UsbOutEvent::PowerTransitionDone(PowerLevel::LowPower))
            //    .await;
            //expect_event(
            //    &bluetooth_in,
            //    UsbInEvent::PowerTransition(PowerLevel::LowPower),
            //).await;
            //bluetooth_out
            //    .send(RadioOutEvent::PowerTransitionDone(PowerLevel::LowPower))
            //    .await;
            //expect_event(
            //    &unifying_in,
            //    UsbInEvent::PowerTransition(PowerLevel::LowPower),
            //).await;
            //unifying_out
            //    .send(RadioOutEvent::PowerTransitionDone(PowerLevel::LowPower))
            //    .await;
            // TODO: Revert power level to normal.

            // Test that the low power mode is also entered when no USB connection is present and
            // when the USB profile is selected.
            // TODO
            // TODO: Also test the two ways to exit the low power mode (USB connection, different
            // profile).

            // Test that the low power mode is also entered when there have not been any key state
            // changes for an extended period of time and when USB is not connected.
            timer.call_start.receive().await;
            timer.expected_durations.send(POWER_SAVING_TIMEOUT).await;
            expect_event(&keys_in, KeysInEvent::PowerTransition(PowerLevel::LowPower)).await;
            keys_out
                .send(KeysOutEvent::PowerTransitionDone(PowerLevel::LowPower))
                .await;
            expect_event(&usb_in, UsbInEvent::PowerTransition(PowerLevel::LowPower)).await;
            usb_out
                .send(UsbOutEvent::PowerTransitionDone(PowerLevel::LowPower))
                .await;
            expect_event(
                &bluetooth_in,
                RadioInEvent::PowerTransition(PowerLevel::LowPower),
            )
            .await;
            bluetooth_out
                .send(RadioOutEvent::PowerTransitionDone(PowerLevel::LowPower))
                .await;
            expect_event(
                &unifying_in,
                RadioInEvent::PowerTransition(PowerLevel::LowPower),
            )
            .await;
            unifying_out
                .send(RadioOutEvent::PowerTransitionDone(PowerLevel::LowPower))
                .await;

            // Any key press shall cause the device to return to the normal power mode.
            // TODO

            // Test that shutdown power state events are forwarded to all sinks and to key matrix
            // as well as power supply. The event shall also stop the dispatcher loop.
            power_supply_out
                .send(PowerSupplyState {
                    mode: PowerSupplyMode::Low,
                    battery_charge: 10,
                    usb_connected: false,
                })
                .await;
            expect_event(&usb_in, UsbInEvent::PowerTransition(PowerLevel::Shutdown)).await;
            usb_out
                .send(UsbOutEvent::PowerTransitionDone(PowerLevel::Shutdown))
                .await;
            expect_event(
                &bluetooth_in,
                RadioInEvent::PowerTransition(PowerLevel::Shutdown),
            )
            .await;
            bluetooth_out
                .send(RadioOutEvent::PowerTransitionDone(PowerLevel::Shutdown))
                .await;
            expect_event(
                &unifying_in,
                RadioInEvent::PowerTransition(PowerLevel::Shutdown),
            )
            .await;
            unifying_out
                .send(RadioOutEvent::PowerTransitionDone(PowerLevel::Shutdown))
                .await;
        };

        with_timeout(
            Duration::from_secs(5),
            futures::future::join(dispatcher_function, test_function),
        )
        .await
        .expect("run() did not stop");
    }

    /// Test whether the device is shut down when the USB profile is selected but USB is not
    /// connected.
    ///
    /// In this case, keeping the device awake would only drain the batteries.
    #[futures_test::test]
    async fn test_usb_disconnected_shutdown() {
        // TODO
    }

    // TODO: Additional tested behavior:
    // - Special keys cause the radio profiles to be switched between bluetooth and unifying.
    // - If the initial data indicates that the keyboard should already be shut down, the
    //   dispatcher immediately returns.
}
