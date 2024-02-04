use defmt::Format;

use super::scancode::ScanCode;

/// Bitmap containing the state of all keys indexed by HID scan code.
#[derive(Debug, PartialEq, Clone, Format)]
pub struct KeyState {
    keys: Bitmap<256>,
}

impl KeyState {
    /// Creates an empty bitmap with no bits set.
    pub fn new() -> KeyState {
        KeyState {
            keys: Bitmap::new(),
        }
    }

    /// Marks a single key as pressed.
    pub fn set_key(&mut self, key: ScanCode) {
        self.keys.set(key as usize);
    }

    /// Marks a single key as released.
    pub fn clear_key(&mut self, key: ScanCode) {
        self.keys.clear(key as usize);
    }

    /// Tests whether a single key is pressed.
    pub fn key_is_set(&mut self, key: ScanCode) -> bool {
        self.keys.bit_is_set(key as usize)
    }

    pub(crate) fn select(
        bitmap: &Bitmap<256>,
        if_not_set: &KeyState,
        if_set: &KeyState,
    ) -> KeyState {
        let mut key_state = KeyState::new();
        for i in 0..8 {
            key_state.keys.data[i] = (bitmap.data[i] & if_set.keys.data[i])
                | (!bitmap.data[i] & if_not_set.keys.data[i]);
        }
        key_state
    }

    pub(crate) fn get_changes(&self, other: &KeyState) -> Bitmap<256> {
        self.keys.xor(&other.keys)
    }

    pub fn to_6kro(&self) -> SixKeySet {
        let mut six_keys = [0u8; 8];
        // The modifier byte is found at position 0xe0 (KEY_LCTRL) in
        // the bitmap and can just be copied.
        let mod_word = ScanCode::LeftControl as usize >> 3;
        six_keys[0] = self.keys.as_bytes()[mod_word];

        // Scan the bitmap and return the first six keys.
        let mut count = 0;
        for key in self.keys.into_iter() {
            if key >= ScanCode::LeftControl as usize {
                break;
            }
            six_keys[2 + count] = key as u8;
            count += 1;
            if count == 6 {
                break;
            }
        }

        return six_keys;
    }
}

impl<'a> IntoIterator for &'a KeyState {
    type Item = ScanCode;
    type IntoIter = KeyIter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        KeyIter {
            bitmap_iter: self.keys.into_iter(),
        }
    }
}

pub struct KeyIter<'a> {
    bitmap_iter: BitmapIter<'a, 256>,
}

impl<'a> Iterator for KeyIter<'a> {
    type Item = ScanCode;

    fn next(&mut self) -> Option<Self::Item> {
        match self.bitmap_iter.next() {
            Some(key) => Some(unsafe { core::mem::transmute(key as u8) }),
            None => None,
        }
    }
}

/// Set of a modifier byte and up to 6 pressed scan codes as required for the HID boot protocol.
pub type SixKeySet = [u8; 8];

#[derive(Debug, PartialEq, Clone, Format)]
pub(crate) struct Bitmap<const SIZE: usize>
where
    [u32; (SIZE + 31) / 32]: Sized,
{
    data: [u32; (SIZE + 31) / 32],
}

impl<const SIZE: usize> Bitmap<SIZE>
where
    [u32; (SIZE + 31) / 32]: Sized,
{
    pub(crate) fn new() -> Self {
        Self {
            data: [0; (SIZE + 31) / 32],
        }
    }

    fn set(&mut self, bit: usize) {
        let word = bit >> 5;
        if word >= self.data.len() {
            return;
        }
        let bit = bit & 0x1f;
        self.data[word] |= 1 << bit;
    }

    fn clear(&mut self, bit: usize) {
        let word = bit >> 5;
        if word >= self.data.len() {
            return;
        }
        let bit = bit & 0x1f;
        self.data[word] &= !(1 << bit);
    }

    fn bit_is_set(&mut self, bit: usize) -> bool {
        let word = bit >> 5;
        if word >= self.data.len() {
            return false;
        }
        let bit = bit & 0x1f;
        (self.data[word] & (1 << bit)) != 0
    }

    pub(crate) fn or(&self, other: &Bitmap<SIZE>) -> Bitmap<SIZE> {
        let mut bitmap = self.clone();
        for i in 0..bitmap.data.len() {
            bitmap.data[i] |= other.data[i];
        }
        bitmap
    }

    fn xor(&self, other: &Bitmap<SIZE>) -> Bitmap<SIZE> {
        let mut bitmap = self.clone();
        for i in 0..bitmap.data.len() {
            bitmap.data[i] ^= other.data[i];
        }
        bitmap
    }

    pub(crate) fn any_bit_set(&self) -> bool {
        self.data != [0; (SIZE + 31) / 32]
    }

    fn as_bytes<'a>(&'a self) -> &'a [u8] {
        unsafe { core::slice::from_raw_parts(&self.data as *const _ as *const u8, SIZE * 4) }
    }
}

impl<'a, const SIZE: usize> IntoIterator for &'a Bitmap<SIZE>
where
    [u32; (SIZE + 31) / 32]: Sized,
{
    type Item = usize;
    type IntoIter = BitmapIter<'a, { SIZE }>;

    fn into_iter(self) -> Self::IntoIter {
        BitmapIter {
            bitmap: self,
            pos: 0,
        }
    }
}

// TODO: Should not be public?
pub struct BitmapIter<'a, const SIZE: usize>
where
    [u32; (SIZE + 31) / 32]: Sized,
{
    bitmap: &'a Bitmap<SIZE>,
    pos: usize,
}

impl<'a, const SIZE: usize> Iterator for BitmapIter<'a, SIZE>
where
    [u32; (SIZE + 31) / 32]: Sized,
{
    type Item = usize;

    fn next(&mut self) -> Option<Self::Item> {
        while self.pos < SIZE {
            let word = self.pos >> 5;
            let bit = self.pos & 0x1f;
            let bits = self.bitmap.data[word] >> bit;
            if bits == 0 {
                // No bit found in this word, try the next.
                self.pos = (word + 1) << 5;
                continue;
            }
            let next_bit = self.pos + bits.trailing_zeros() as usize;
            self.pos = next_bit + 1;
            return Some(next_bit);
        }
        None
    }
}

#[allow(async_fn_in_trait)]
pub trait Timer {
    async fn after(&self, duration: embassy_time::Duration);
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::vec::Vec;

    #[test]
    fn test_bitmap() {
        // Test the initial state.
        // TODO: Test other sizes? Not needed in this firmware.
        let mut bitmap = Bitmap::<256>::new();

        assert_eq!(bitmap.data, [0; 8], "bitmap is initially empty");
        assert_eq!(
            bitmap.into_iter().collect::<Vec<_>>(),
            vec![],
            "iterating over empty bitmap failed"
        );

        // Test setting bits.
        bitmap.set(0);
        bitmap.set(29);
        bitmap.set(31);
        bitmap.set(33);
        bitmap.set(255);
        bitmap.set(256);
        bitmap.set(100000);
        let good = [0xa0000001, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80000000];
        assert_eq!(bitmap.data, good, "correct bits are set");

        // Test iterating over bits.
        let good = vec![0, 29, 31, 33, 255];
        let bits_set = bitmap.into_iter().collect::<Vec<_>>();
        assert_eq!(bits_set, good, "iterating over bitmap failed");

        // Test bit_is_set().
        assert_eq!(bitmap.bit_is_set(0), true, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(1), false, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(29), true, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(30), false, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(31), true, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(32), false, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(33), true, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(255), true, "bit_is_set failed");
        assert_eq!(bitmap.bit_is_set(256), false, "bit_is_set failed");

        // Test clearing bits.
        bitmap.clear(255);
        assert_eq!(bitmap.data[7], 0, "correct bit was cleared");
        assert_eq!(
            bitmap.into_iter().collect::<Vec<_>>(),
            vec![0, 29, 31, 33],
            "iterating over bitmap failed after clearing"
        );
        bitmap.data[1] = 0xdeadc0de;
        bitmap.clear(35);
        assert_eq!(bitmap.data[1], 0xdeadc0d6, "correct bit was cleared");
        bitmap.clear(29);
        assert_eq!(bitmap.data[0], 0x80000001, "correct bit was cleared");
        bitmap.clear(100000);
    }

    #[test]
    fn six_key_set_test() {
        // Test that keys are correctly collected from a bitmap.
        let mut bitmap = KeyState::new();
        let six_keys = bitmap.to_6kro();
        let mut good = [0, 0, 0, 0, 0, 0, 0, 0];
        assert_eq!(six_keys, good, "empty bitmap produced wrong SixKeySet");
        bitmap.set_key(ScanCode::X);
        let six_keys = bitmap.to_6kro();
        good[2] = ScanCode::X as u8;
        assert_eq!(six_keys, good, "single key produced wrong SixKeySet",);
        bitmap.set_key(ScanCode::Escape);
        bitmap.set_key(ScanCode::A);
        bitmap.set_key(ScanCode::C);
        let six_keys = bitmap.to_6kro();
        good[2] = ScanCode::A as u8;
        good[3] = ScanCode::C as u8;
        good[4] = ScanCode::X as u8;
        good[5] = ScanCode::Escape as u8;
        assert_eq!(six_keys, good, "multiple keys produced wrong SixKeySet",);
        bitmap.clear_key(ScanCode::A);
        bitmap.set_key(ScanCode::D);
        bitmap.set_key(ScanCode::E);
        bitmap.set_key(ScanCode::F);
        let six_keys = bitmap.to_6kro();
        good[2] = ScanCode::C as u8;
        good[3] = ScanCode::D as u8;
        good[4] = ScanCode::E as u8;
        good[5] = ScanCode::F as u8;
        good[6] = ScanCode::X as u8;
        good[7] = ScanCode::Escape as u8;
        assert_eq!(six_keys, good, "6 keys produced wrong SixKeySet",);
        bitmap.set_key(ScanCode::G);
        let six_keys = bitmap.to_6kro();
        good[6] = ScanCode::G as u8;
        good[7] = ScanCode::X as u8;
        assert_eq!(six_keys, good, "7 keysproduced wrong SixKeySet",);
        bitmap.clear_key(ScanCode::D);
        bitmap.clear_key(ScanCode::E);
        bitmap.clear_key(ScanCode::F);
        let six_keys = bitmap.to_6kro();
        good[2] = ScanCode::C as u8;
        good[3] = ScanCode::G as u8;
        good[4] = ScanCode::X as u8;
        good[5] = ScanCode::Escape as u8;
        good[6] = 0;
        good[7] = 0;
        assert_eq!(six_keys, good, "4 keys produced wrong SixKeySet",);

        // Test that modifier keys are only collected as part of the
        // modifier byte.
        bitmap.set_key(ScanCode::LeftControl);
        let six_keys = bitmap.to_6kro();
        good[0] = 0x1;
        assert_eq!(six_keys, good, "left control produced wrong SixKeySet",);
        bitmap.set_key(ScanCode::RightAlt);
        let six_keys = bitmap.to_6kro();
        good[0] = 0x41;
        assert_eq!(six_keys, good, "lctrl+alt produced wrong SixKeySet",);
        bitmap.set_key(ScanCode::RightMeta);
        let six_keys = bitmap.to_6kro();
        good[0] = 0xC1;
        assert_eq!(six_keys, good, "lctrl+alt+rmeta produced wrong SixKeySet",);
        bitmap.clear_key(ScanCode::RightMeta);
        bitmap.clear_key(ScanCode::RightAlt);
        bitmap.clear_key(ScanCode::LeftControl);
        let six_keys = bitmap.to_6kro();
        good[0] = 0x0;
        assert_eq!(
            six_keys, good,
            "empty control byte produced wrong SixKeySet",
        );
    }
}
