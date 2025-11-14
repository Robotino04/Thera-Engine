use std::ops::{Deref, DerefMut};

use crate::piece::Square;

#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub struct Bitboard(u64);

impl DerefMut for Bitboard {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl Deref for Bitboard {
    type Target = u64;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl Bitboard {
    pub const fn at(&self, index: Square) -> bool {
        (self.0 >> index as u8) & 0b1 == 1
    }
    pub const fn set(&mut self, index: Square) {
        self.0 |= 1 << index as u8
    }
    pub const fn clear(&mut self, index: Square) {
        self.0 &= !(1 << index as u8)
    }
    pub const fn assign(&mut self, index: Square, value: bool) {
        self.clear(index);
        self.0 |= (value as u64) << (index as u8)
    }
}
