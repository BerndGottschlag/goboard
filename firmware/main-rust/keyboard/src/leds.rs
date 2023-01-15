// TODO: What about num lock?

pub trait LedPins {
    fn disable();

    fn set_caps_lock(state: bool),
    fn set_scroll_lock(state: bool),
    fn set_power_sequence(sequence: [u16; 128]);
}
