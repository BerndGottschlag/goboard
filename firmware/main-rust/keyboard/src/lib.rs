//! TODO: Documentation
//!
//! - Modelled as async procedures
//! - Problem, concurrency, interaction with HW often hard to make cancel-safe
//! - Therefore: weak coupling via cancel-safe channels and asynchronous tasks

#![allow(incomplete_features)]
#![feature(generic_const_exprs, async_closure)]
#![no_std]

pub mod dispatcher;
mod key_state;
pub mod keys;
pub mod mode_switch;
pub mod power_supply;
pub mod scancode;
mod select6;

pub use key_state::*;

#[cfg(test)]
#[macro_use]
extern crate std;

// defmt does not work on Linux, so we use log for the tests.
#[cfg(not(test))]
pub use defmt::{debug, error, info, trace, warn};
#[cfg(test)]
pub use log::{debug, error, info, trace, warn};

#[cfg(not(target_os = "none"))]
#[defmt::panic_handler]
fn defmt_panic() -> ! {
    panic!("defmt panic");
}

#[cfg(not(target_os = "none"))]
#[defmt::global_logger]
struct Logger;

#[cfg(not(target_os = "none"))]
unsafe impl defmt::Logger for Logger {
    fn acquire() {}
    unsafe fn flush() {}
    unsafe fn release() {}
    unsafe fn write(_bytes: &[u8]) {
        panic!("did not expect defmt output");
    }
}

#[cfg(not(target_os = "none"))]
defmt::timestamp!("{=usize}", 0);

#[derive(Debug, PartialEq, Clone, defmt::Format)]
pub enum PowerLevel {
    Normal,
    LowPower,
    Shutdown,
}

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
            let expected = self.expected_durations.receive().await;
            assert_eq!(duration, expected);
        }
    }

    pub struct NopMockTimer;

    impl Timer for NopMockTimer {
        async fn after(&self, _duration: Duration) {}
    }
}
