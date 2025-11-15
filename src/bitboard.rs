use std::ops::{BitAnd, BitAndAssign, BitOr, BitOrAssign, Deref, DerefMut, Not};

use crate::piece::Square;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Bitboard(pub u64);

impl std::fmt::Debug for Bitboard {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "Bitboard({:#016x}){{", self.0)?;
        for rank in 0..8 {
            write!(f, "    ")?;
            for file in 0..8 {
                let sq = rank * 8 + file;
                let bit = (self.0 >> sq) & 0b1;
                write!(f, "{} ", bit)?;
            }
            writeln!(f)?;
        }
        write!(f, "}}")
    }
}

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

impl Not for Bitboard {
    type Output = Self;

    fn not(self) -> Self::Output {
        Bitboard(!self.0)
    }
}

impl BitAnd for Bitboard {
    type Output = Self;

    fn bitand(self, rhs: Self) -> Self::Output {
        Bitboard(self.0 & rhs.0)
    }
}

impl BitAndAssign for Bitboard {
    fn bitand_assign(&mut self, rhs: Self) {
        self.0 &= rhs.0
    }
}
impl BitOr for Bitboard {
    type Output = Self;

    fn bitor(self, rhs: Self) -> Self::Output {
        Bitboard(self.0 | rhs.0)
    }
}

impl BitOrAssign for Bitboard {
    fn bitor_assign(&mut self, rhs: Self) {
        self.0 |= rhs.0
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

    pub const fn from_square(square: Square) -> Bitboard {
        Bitboard(1 << (square as u8))
    }

    pub const fn is_empty(&self) -> bool {
        self.0 == 0
    }
}
