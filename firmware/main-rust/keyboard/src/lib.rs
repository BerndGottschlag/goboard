//! TODO: Documentation
//!
//! - Modelled as async procedures
//! - Problem, concurrency, interaction with HW often hard to make cancel-safe
//! - Therefore: weak coupling via cancel-safe channels and asynchronous tasks

#![allow(incomplete_features)]
#![feature(async_fn_in_trait, generic_const_exprs, async_closure)]
#![no_std]

mod key_state;
pub mod keys;
pub mod mode_switch;
pub mod power_supply;
pub mod scancode;

pub use key_state::*;

#[cfg(test)]
#[macro_use]
extern crate std;

/*#[derive(Debug, PartialEq, Clone, Format)]
pub enum LedEvent {
    PowerTransition(PowerLevel),
    CapsLockLed(bool),
    NumLockLed(bool),
    ScrollLockLed(bool),
    Charging(bool), // Pulsing, "breathing" animation
    Connection(ConnectionStatus),
}*/

/*#[derive(Debug, PartialEq, Clone, Format)]
pub enum ConnectionStatus {
    Disconnected,
    Pairing,      // Rapid blinking.
    Reconnecting, // Slower blinking.
    Connected,
}*/

#[cfg(test)]
mod test_utils {
    use super::Timer;

    use embassy_sync::blocking_mutex::raw::NoopRawMutex;
    use embassy_sync::channel::Channel;
    use embassy_time::Duration;

    pub struct MockTimer {
        pub call_start: Channel<NoopRawMutex, (), 1>,
        pub expected_durations: Channel<NoopRawMutex, Duration, 1>,
    }

    impl MockTimer {
        pub fn new() -> Self {
            Self {
                call_start: Channel::new(),
                expected_durations: Channel::new(),
            }
        }
    }

    impl Timer for MockTimer {
        async fn after(&self, duration: Duration) {
            self.call_start.send(()).await;
            let expected = self.expected_durations.recv().await;
            assert_eq!(duration, expected);
        }
    }

    pub struct NopMockTimer;

    impl Timer for NopMockTimer {
        async fn after(&self, _duration: Duration) {}
    }
}
