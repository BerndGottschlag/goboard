#![allow(incomplete_features)]
#![feature(async_fn_in_trait, generic_const_exprs, async_closure)]
#![no_std]

mod key_state;
pub mod keys;
pub mod scancode;

pub use key_state::*;

#[cfg(test)]
#[macro_use]
extern crate std;

// TODO: Place different events into different channels for type-safe event delivery?

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
