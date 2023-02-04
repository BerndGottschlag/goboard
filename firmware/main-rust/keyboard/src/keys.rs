use super::scancode::ScanCode;
use super::{Bitmap, KeyState, PowerLevel, Timer};

use defmt::{warn, Format};
use embassy_sync::blocking_mutex::raw::NoopRawMutex;
use embassy_sync::channel::{Receiver, Sender};
use embassy_time::Duration;

pub trait KeyMatrixHardware {
    /// Enables the key matrix.
    ///
    /// This function enables power to the key matrix.
    async fn enable(&mut self);

    /// Disables the key matrix and removes power from the shift registers.
    fn disable(&mut self);

    /// First sets the row selection shift register to read the specified row, then reads the input
    /// shift register.
    async fn read_row(&mut self, row_bitmap: u8) -> u16;
}

pub trait NumpadConnection {}

#[derive(Debug, PartialEq, Clone, Format)]
pub enum KeysInEvent {
    NumlockLed(bool),
    PowerTransition(PowerLevel),
}

#[derive(Debug, PartialEq, Clone, Format)]
pub enum KeysOutEvent {
    KeysChanged(KeyState),
    PowerTransition(PowerLevel),
    PowerTransitionDone(PowerLevel),
}

/// Key state collection and debouncing.
///
/// This class collects all keys from the main key matrix and the numpad, and it
/// implements 5ms switch debouncing for the former. The class also implements
/// interpretation of FN key combinations.
pub struct Keys<'a, KeyMatrix: KeyMatrixHardware> {
    key_matrix: KeyMatrix,
    events_in: Receiver<'a, NoopRawMutex, KeysInEvent, 1>,
    events_out: Sender<'a, NoopRawMutex, KeysOutEvent, 1>,
    bitmap_debounced: KeyState,
    key_changes: [Bitmap<256>; 5],
    reported: KeyState,
}

impl<'a, KeyMatrix: KeyMatrixHardware> Keys<'a, KeyMatrix> {
    pub async fn new(
        mut key_matrix: KeyMatrix,
        events_in: Receiver<'a, NoopRawMutex, KeysInEvent, 1>,
        events_out: Sender<'a, NoopRawMutex, KeysOutEvent, 1>,
    ) -> Keys<'a, KeyMatrix> {
        key_matrix.enable().await;
        Keys {
            key_matrix,
            events_in,
            events_out,
            bitmap_debounced: KeyState::new(),
            key_changes: [
                Bitmap::new(),
                Bitmap::new(),
                Bitmap::new(),
                Bitmap::new(),
                Bitmap::new(),
            ],
            reported: KeyState::new(),
        }
    }

    /// Polls all keys and applies debouncing.
    ///
    /// interval_ms must contain the milliseconds since the last call to `poll()`.
    async fn poll(&mut self, interval_ms: u32) {
        // Read the matrix:
        let mut key_state_bouncy = KeyState::new();
        for row in 0..ROWS {
            let row_pattern = 1 << row;
            let row_data = self.key_matrix.read_row(row_pattern).await;

            // Map keys.
            for column in 0..16 {
                if (row_data & (1 << column)) != 0 {
                    key_state_bouncy.set_key(KEY_MATRIX_LOCATIONS[row][column]);
                }
            }
        }

        // We implement debouncing via a simple algorithm that tracks changes in the resulting key
        // state. The changes during the last five milliseconds are combined, and changes to the
        // bitmap reported by the key matrix are ignored if the same key already changed its state
        // during that time. For those keys which recently changed, the value from the previous
        // debounced key state is taken instead.

        // Key state for bits that changed.
        let bitmap_debounced_old = self.bitmap_debounced.clone();

        // TODO: The following access patterns can probably be optimized for better register usage
        // (flip inner and outer loops).
        let times_to_shift = if interval_ms >= 5 { 5 } else { interval_ms };
        for _ in 0..times_to_shift {
            self.key_changes[4] = self.key_changes[3].clone();
            self.key_changes[3] = self.key_changes[2].clone();
            self.key_changes[2] = self.key_changes[1].clone();
            self.key_changes[1] = self.key_changes[0].clone();
            self.key_changes[0] = Bitmap::new();
        }

        let changed = self.key_changes[1]
            .or(&self.key_changes[2])
            .or(&self.key_changes[3])
            .or(&self.key_changes[4]);
        self.bitmap_debounced =
            KeyState::select(&changed, &key_state_bouncy, &bitmap_debounced_old);
        self.key_changes[0] = self.bitmap_debounced.get_changes(&bitmap_debounced_old);

        // If we have any changes, we need to trigger a key state change event.
        if self.key_changes[0].any_bit_set() {
            // We have to apply FN key mappings as the rest of the code expects us to generate
            // special keys.
            let mut mapped = self.bitmap_debounced.clone();
            if mapped.key_is_set(ScanCode::Function) {
                mapped.clear_key(ScanCode::Function);

                for f_key_code in ScanCode::F1 as usize..=ScanCode::F12 as usize {
                    let f_key = unsafe { core::mem::transmute(f_key_code as u8) };
                    if mapped.key_is_set(f_key) {
                        mapped.clear_key(f_key);
                        mapped.set_key(F_FN_MAPPING[f_key_code - ScanCode::F1 as usize]);
                    }
                }
            }

            // We also must not report invalid keys.
            mapped.clear_key(ScanCode::Invalid);

            // We must not report changes if only the fn key changes.
            if self.reported.get_changes(&mapped).any_bit_set() {
                self.reported = mapped.clone();
                self.events_out
                    .send(KeysOutEvent::KeysChanged(mapped))
                    .await;
            }
        }
    }

    pub async fn run<Sleep: Timer>(&mut self, timer: &Sleep) {
        let mut interval = Duration::from_millis(1);
        loop {
            // poll() is not cancel-safe, so we only check the power state inbetween invocations.
            self.poll(1).await;

            while let Ok(event) = self.events_in.try_recv() {
                match event {
                    KeysInEvent::PowerTransition(power_level) => match power_level {
                        PowerLevel::Normal => {
                            interval = Duration::from_millis(1);
                            self.events_out
                                .send(KeysOutEvent::PowerTransitionDone(power_level))
                                .await;
                        }
                        PowerLevel::LowPower => {
                            interval = Duration::from_millis(10);
                            self.events_out
                                .send(KeysOutEvent::PowerTransitionDone(power_level))
                                .await;
                        }
                        PowerLevel::Shutdown => {
                            self.key_matrix.disable();
                            self.events_out
                                .send(KeysOutEvent::PowerTransitionDone(power_level))
                                .await;
                            return;
                        }
                    },
                    KeysInEvent::NumlockLed(_numlock) => {
                        // TODO: Send message to numpad if available, store locally if not connected.
                        warn!("Numlock LED not yet implemented!");
                    }
                }
            }

            timer.after(interval).await;
        }
    }
}

const ROWS: usize = 6;
const COLUMNS: usize = 16;
const KEY_MATRIX_LOCATIONS: [[ScanCode; COLUMNS]; ROWS] = [
    [
        ScanCode::Insert,
        ScanCode::Delete,
        ScanCode::Equal,
        ScanCode::Backspace,
        ScanCode::Minus,
        ScanCode::Digit0,
        ScanCode::Digit8,
        ScanCode::Digit9,
        ScanCode::Digit7,
        ScanCode::Digit6,
        ScanCode::Digit5,
        ScanCode::Digit4,
        ScanCode::Digit3,
        ScanCode::Digit2,
        ScanCode::Backtick,
        ScanCode::Digit1,
    ],
    [
        ScanCode::Pause,
        ScanCode::ScrollLock,
        ScanCode::F12,
        ScanCode::Print,
        ScanCode::F11,
        ScanCode::F10,
        ScanCode::F8,
        ScanCode::F9,
        ScanCode::F7,
        ScanCode::F6,
        ScanCode::F5,
        ScanCode::F4,
        ScanCode::F3,
        ScanCode::F2,
        ScanCode::Escape,
        ScanCode::F1,
    ],
    [
        ScanCode::Invalid,
        ScanCode::Invalid,
        ScanCode::RightBracket,
        ScanCode::Return,
        ScanCode::LeftBracket,
        ScanCode::P,
        ScanCode::I,
        ScanCode::O,
        ScanCode::U,
        ScanCode::Y,
        ScanCode::T,
        ScanCode::R,
        ScanCode::E,
        ScanCode::W,
        ScanCode::Tab,
        ScanCode::Q,
    ],
    [
        ScanCode::PageUp,
        ScanCode::Invalid,
        ScanCode::Backslash,
        ScanCode::Invalid,
        ScanCode::Quote,
        ScanCode::Semicolon,
        ScanCode::K,
        ScanCode::L,
        ScanCode::J,
        ScanCode::H,
        ScanCode::G,
        ScanCode::F,
        ScanCode::D,
        ScanCode::S,
        ScanCode::CapsLock,
        ScanCode::A,
    ],
    [
        ScanCode::Right,
        ScanCode::Down,
        ScanCode::RightControl,
        ScanCode::Left,
        ScanCode::LeftMeta,
        ScanCode::RightAlt,
        ScanCode::Invalid,
        ScanCode::Invalid,
        ScanCode::Invalid,
        ScanCode::Space,
        ScanCode::Invalid,
        ScanCode::Invalid,
        ScanCode::Invalid,
        ScanCode::LeftAlt,
        ScanCode::LeftControl,
        ScanCode::LeftMeta,
    ],
    [
        ScanCode::PageDown,
        ScanCode::Up,
        ScanCode::RightShift,
        ScanCode::Invalid,
        ScanCode::Slash,
        ScanCode::Period,
        ScanCode::M,
        ScanCode::Comma,
        ScanCode::N,
        ScanCode::B,
        ScanCode::V,
        ScanCode::C,
        ScanCode::X,
        ScanCode::Z,
        ScanCode::LeftShift,
        ScanCode::Function,
    ],
];

const F_FN_MAPPING: [ScanCode; 12] = [
    ScanCode::FunctionPair,
    ScanCode::FunctionBluetooth,
    ScanCode::FunctionUnifying,
    ScanCode::FunctionUnpairAll,
    ScanCode::F4,
    ScanCode::F5,
    ScanCode::F6,
    ScanCode::F7,
    ScanCode::FunctionToggleGameMode,
    ScanCode::Mute,
    ScanCode::VolumeUp,
    ScanCode::VolumeDown,
];

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_utils::MockTimer;

    use embassy_sync::channel::Channel;

    use std::sync::Mutex;
    use std::vec::Vec;

    /// Artificial key matrix which allows setting the key state as part of
    /// tests.
    struct MockKeyMatrix {
        data: Mutex<MockKeyMatrixData>,
    }

    impl MockKeyMatrix {
        fn new() -> MockKeyMatrix {
            MockKeyMatrix {
                data: Mutex::new(MockKeyMatrixData {
                    enabled: false,
                    matrix_state: [0; 6],
                }),
            }
        }

        fn set_single_key(&self, row: usize, column: u16) {
            let mut data = self.data.lock().unwrap();
            data.clear();
            data.matrix_state[row] = 1 << column;
        }

        fn set_two_keys(&self, row1: usize, column1: u16, row2: usize, column2: u16) {
            let mut data = self.data.lock().unwrap();
            data.clear();
            data.matrix_state[row1] = 1 << column1;
            data.matrix_state[row2] = 1 << column2;
        }

        fn set_key(&self, row: usize, column: u16) {
            let mut data = self.data.lock().unwrap();
            data.matrix_state[row] = 1 << column;
        }

        fn clear(&self) {
            let mut data = self.data.lock().unwrap();
            data.clear();
        }
    }

    impl KeyMatrixHardware for &MockKeyMatrix {
        async fn enable(&mut self) {
            let mut data = self.data.lock().unwrap();
            data.enabled = true;
        }

        fn disable(&mut self) {
            let mut data = self.data.lock().unwrap();
            data.enabled = false;
        }

        async fn read_row(&mut self, row_bitmap: u8) -> u16 {
            let data = self.data.lock().unwrap();
            assert!(data.enabled, "select_row() on inactive matrix");
            assert!(row_bitmap.count_ones() == 1, "not exactly one row selected");
            let selected_row = row_bitmap.trailing_zeros() as i32;
            data.matrix_state[selected_row as usize]
        }
    }

    struct MockKeyMatrixData {
        enabled: bool,
        matrix_state: [u16; 6],
    }

    impl MockKeyMatrixData {
        fn clear(&mut self) {
            for row in 0..6 {
                self.matrix_state[row] = 0;
            }
        }
    }

    fn expect_no_key_event(output: &Channel<NoopRawMutex, KeysOutEvent, 1>) {
        if output.try_recv().is_ok() {
            panic!("received key state change event, but no key changed");
        }
    }

    fn expect_key_event(output: &Channel<NoopRawMutex, KeysOutEvent, 1>, keys: &[ScanCode]) {
        let event = output
            .try_recv()
            .expect("did not generate key change event");
        if let KeysOutEvent::KeysChanged(key_state) = event {
            assert_eq!(
                &key_state.into_iter().collect::<Vec<_>>(),
                keys,
                "wrong keys received"
            );
        } else {
            panic!("wrong event received");
        }
    }

    #[futures_test::test]
    async fn test_key_mapping() {
        for row in 0..6 {
            for column in 0..16 {
                let scan_code = KEY_MATRIX_LOCATIONS[row][column];
                let key_matrix = MockKeyMatrix::new();
                key_matrix.set_single_key(row, column as u16);

                let input = Channel::new();
                let output = Channel::new();
                let mut keys = Keys::new(&key_matrix, input.receiver(), output.sender()).await;

                keys.poll(1).await;
                if scan_code == ScanCode::Function || scan_code == ScanCode::Invalid {
                    // Not reported.
                    expect_no_key_event(&output);
                } else {
                    expect_key_event(&output, &[scan_code]);
                }
            }
        }
    }

    #[futures_test::test]
    async fn test_debouncing() {
        let row = 2;
        let column = 5;
        let scan_code = KEY_MATRIX_LOCATIONS[row][column as usize];
        let row2 = 3;
        let column2 = 7;
        let scan_code2 = KEY_MATRIX_LOCATIONS[row2][column2 as usize];

        let key_matrix = MockKeyMatrix::new();
        let input = Channel::new();
        let output = Channel::new();
        let mut keys = Keys::new(&key_matrix, input.receiver(), output.sender()).await;

        keys.poll(1).await;
        expect_no_key_event(&output);

        // Correct timing when keys are changed with more than 5ms
        // inbetween changes.
        key_matrix.set_single_key(row, column);
        keys.poll(1).await;
        expect_key_event(&output, &[scan_code]);
        for _ in 0..4 {
            keys.poll(1).await;
            expect_no_key_event(&output);
        }
        key_matrix.clear();
        keys.poll(1).await;
        expect_key_event(&output, &[]);
        for _ in 0..4 {
            keys.poll(1).await;
            expect_no_key_event(&output);
        }
        key_matrix.set_single_key(row, column);

        // Longer intervals.
        keys.poll(1).await;
        expect_key_event(&output, &[scan_code]);
        key_matrix.clear();
        keys.poll(5).await;
        expect_key_event(&output, &[]);
        key_matrix.set_single_key(row, column);
        keys.poll(6).await;
        expect_key_event(&output, &[scan_code]);

        // Test whether bouncing is ignored.
        key_matrix.clear();
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_key_event(&output, &[]);

        key_matrix.set_single_key(row, column);
        keys.poll(1).await;
        expect_no_key_event(&output);
        key_matrix.clear();
        keys.poll(1).await;
        expect_no_key_event(&output);
        key_matrix.set_single_key(row, column);
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_no_key_event(&output);
        keys.poll(1).await;
        expect_key_event(&output, &[scan_code]);

        // Test whether bouncing is ignored for multiple keys.
        key_matrix.set_two_keys(row, column, row2, column2);
        keys.poll(2).await;
        expect_key_event(&output, &[scan_code2, scan_code]);
        key_matrix.clear();
        keys.poll(2).await;
        expect_no_key_event(&output);
        keys.poll(2).await;
        expect_key_event(&output, &[scan_code2]);
        keys.poll(1).await;
        expect_key_event(&output, &[]);
    }

    #[futures_test::test]
    async fn test_fn_combinations() {
        // Test mapping of all FN key combinations.
        let f_row = 1;
        let f_columns = [15, 13, 12, 11, 10, 9, 8, 6, 7, 5, 4, 2];
        let fn_row = 5;
        let fn_column = 15;
        for i in 0..12 {
            let key_matrix = MockKeyMatrix::new();
            key_matrix.set_two_keys(fn_row, fn_column, f_row, f_columns[i]);

            let input = Channel::new();
            let output = Channel::new();
            let mut keys = Keys::new(&key_matrix, input.receiver(), output.sender()).await;

            keys.poll(1).await;
            if F_FN_MAPPING[i] == ScanCode::Invalid {
                // Not reported.
                expect_no_key_event(&output);
            } else {
                expect_key_event(&output, &[F_FN_MAPPING[i]]);
            }
        }

        // If multiple keys are pressed while FN is pressed, only the F
        // keys are translated.
        let a_row = 3;
        let a_column = 15;
        let key_matrix = MockKeyMatrix::new();
        key_matrix.clear();
        key_matrix.set_key(fn_row, fn_column);
        key_matrix.set_key(f_row, f_columns[0]);
        key_matrix.set_key(a_row, a_column);

        let input = Channel::new();
        let output = Channel::new();
        let mut keys = Keys::new(&key_matrix, input.receiver(), output.sender()).await;

        keys.poll(1).await;
        expect_key_event(&output, &[ScanCode::A, F_FN_MAPPING[0]]);
    }

    #[futures_test::test]
    async fn test_numpad() {
        // TODO
    }

    fn expect_power_transition_done(
        output: &Channel<NoopRawMutex, KeysOutEvent, 1>,
        level: PowerLevel,
    ) {
        let event = output
            .try_recv()
            .expect("did not generate key change event");
        assert_eq!(
            event,
            KeysOutEvent::PowerTransitionDone(level),
            "expected power transition done event"
        );
    }

    #[futures_test::test]
    async fn test_run() {
        let key_matrix = MockKeyMatrix::new();
        let timer = MockTimer::new();

        let input = Channel::new();
        let output = Channel::new();

        let keys_function = async {
            let mut keys = Keys::new(&key_matrix, input.receiver(), output.sender()).await;

            keys.run(&timer).await;
        };

        let test_function = async {
            // Test that run() causes key state updates to be emitted.
            timer.call_start.recv().await;
            let a_row = 3;
            let a_column = 15;
            key_matrix.set_single_key(a_row, a_column as u16);
            timer
                .expected_durations
                .send(Duration::from_millis(1))
                .await;
            timer.call_start.recv().await;
            expect_key_event(&output, &[ScanCode::A]);

            // Test that run() sleeps for the correct durations.
            input
                .send(KeysInEvent::PowerTransition(PowerLevel::LowPower))
                .await;
            // This sleep is still 1ms long, but the next should be 10ms long.
            timer
                .expected_durations
                .send(Duration::from_millis(1))
                .await;
            timer.call_start.recv().await;
            expect_power_transition_done(&output, PowerLevel::LowPower);
            timer
                .expected_durations
                .send(Duration::from_millis(10))
                .await;
            timer.call_start.recv().await;
            input
                .send(KeysInEvent::PowerTransition(PowerLevel::Normal))
                .await;
            timer
                .expected_durations
                .send(Duration::from_millis(10))
                .await;
            timer.call_start.recv().await;
            expect_power_transition_done(&output, PowerLevel::Normal);
            timer
                .expected_durations
                .send(Duration::from_millis(1))
                .await;

            // Test that run() disables the key matrix on shutdown.
            timer.call_start.recv().await;
            input
                .send(KeysInEvent::PowerTransition(PowerLevel::Shutdown))
                .await;
            timer
                .expected_durations
                .send(Duration::from_millis(1))
                .await;
        };

        futures::future::join(keys_function, test_function).await;

        expect_power_transition_done(&output, PowerLevel::Shutdown);
        assert_eq!(
            key_matrix.data.lock().unwrap().enabled,
            false,
            "key matrix not disabled"
        );
    }
}
