use std::ops::{
    BitAnd, BitAndAssign, BitOr, BitOrAssign, BitXor, BitXorAssign, Deref, DerefMut, Not,
};

use crate::piece::{Direction, Square};

#[derive(Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Bitboard(pub u64);

#[macro_export]
macro_rules! bitboard {
    ($line0:tt $line1:tt $line2:tt $line3:tt $line4:tt $line5:tt $line6:tt $line7:tt) => {
        $crate::bitboard::Bitboard(
            ($line0 << 56)
                | ($line1 << 48)
                | ($line2 << 40)
                | ($line3 << 32)
                | ($line4 << 24)
                | ($line5 << 16)
                | ($line6 << 8)
                | ($line7 << 0),
        )
    };
}
pub use bitboard;

impl std::fmt::Debug for Bitboard {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "Bitboard({:#016x}){{", self.0)?;
        for y in (0..8).rev() {
            write!(f, "    ")?;
            for x in 0..8 {
                let bit = self.at(Square::from_xy(x, y).unwrap()) as u8;
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

impl BitXor for Bitboard {
    type Output = Self;

    fn bitxor(self, rhs: Self) -> Self::Output {
        Bitboard(self.0 ^ rhs.0)
    }
}

impl BitXorAssign for Bitboard {
    fn bitxor_assign(&mut self, rhs: Self) {
        self.0 ^= rhs.0
    }
}

impl Bitboard {
    pub const fn from_rows(rows: [u8; 8]) -> Self {
        Bitboard(u64::from_be_bytes(rows))
    }

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

    pub const fn shift(&self, dir: Direction) -> Self {
        self.prevent_wrapping(dir).wrapping_shift(dir, 1)
    }
    pub const fn rotate(&self, dir: Direction, n: i32) -> Self {
        let amount = dir as i32 * n;
        Bitboard(self.0.rotate_left(amount as u32))
    }

    /// NOTE: this is actually just another name for rotate_left.
    /// It should only be used if you called `prevent_wrapping()` on this
    /// value beforehand or if you know that your value won't hit any edge
    /// of the board.
    pub const fn wrapping_shift(&self, dir: Direction, n: i32) -> Self {
        let amount = dir as i32 * n;
        Self(self.0.rotate_left(amount as u32))
    }

    pub const fn bitscan(&mut self) -> Option<Bitboard> {
        if self.0 == 0 {
            None
        } else {
            let mask = self.0 & self.0.wrapping_neg();
            self.0 &= self.0 - 1;
            Some(Bitboard(mask))
        }
    }

    pub const fn bitscan_index(&mut self) -> Option<u32> {
        if self.0 == 0 {
            None
        } else {
            let index = self.0.trailing_zeros();
            self.0 &= self.0 - 1;
            Some(index)
        }
    }
    pub const fn first_piece_index(&self) -> Option<u32> {
        if self.0 == 0 {
            None
        } else {
            Some(self.0.trailing_zeros())
        }
    }

    pub fn first_piece_square(&self) -> Option<Square> {
        self.first_piece_index()
            .map(|x| Square::new(x as u8).unwrap())
    }

    pub const fn const_or(&self, rhs: Bitboard) -> Self {
        Self(self.0 | rhs.0)
    }
    pub const fn const_and(&self, rhs: Bitboard) -> Self {
        Self(self.0 & rhs.0)
    }
    pub const fn const_not(&self) -> Self {
        Self(!self.0)
    }
    pub const fn const_xor(&self, rhs: Bitboard) -> Self {
        Self(self.0 ^ rhs.0)
    }

    pub const fn prevent_wrapping(&self, dir: Direction) -> Bitboard {
        match dir {
            Direction::North => Self(
                self.0
                    & bitboard!(
                        0b_00000000
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                    )
                    .0,
            ),

            Direction::South => Self(
                self.0
                    & bitboard!(
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_11111111
                        0b_00000000
                    )
                    .0,
            ),
            Direction::East => Self(
                self.0
                    & bitboard!(
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                    )
                    .0,
            ),
            Direction::West => Self(
                self.0
                    & bitboard!(
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                    )
                    .0,
            ),
            Direction::NorthEast => Self(
                self.0
                    & bitboard!(
                        0b_00000000
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                    )
                    .0,
            ),
            Direction::NorthWest => Self(
                self.0
                    & bitboard!(
                        0b_00000000
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                    )
                    .0,
            ),
            Direction::SouthEast => Self(
                self.0
                    & bitboard!(
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_11111110
                        0b_00000000
                    )
                    .0,
            ),
            Direction::SouthWest => Self(
                self.0
                    & bitboard!(
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_01111111
                        0b_00000000
                    )
                    .0,
            ),
        }
    }
}

#[cfg(test)]
mod test {
    use crate::{
        bitboard::Bitboard,
        piece::{Direction, Square},
    };

    #[test]
    fn shift_directions() {
        assert_eq!(
            Bitboard::from_square(Square::E3).shift(Direction::North),
            Bitboard::from_square(Square::E4)
        );

        assert_eq!(
            Bitboard::from_square(Square::E3).rotate(Direction::North, 1),
            Bitboard::from_square(Square::E4)
        );
        assert_eq!(
            Bitboard::from_square(Square::E3).rotate(Direction::North, 3),
            Bitboard::from_square(Square::E6)
        );
    }

    #[test]
    fn shift_edge() {
        assert_eq!(
            Bitboard::from_square(Square::B1).shift(Direction::South),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::A1).shift(Direction::SouthWest),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::A6).shift(Direction::West),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::A8).shift(Direction::NorthWest),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::D8).shift(Direction::North),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::H8).shift(Direction::NorthEast),
            Bitboard(0)
        );
        assert_eq!(
            Bitboard::from_square(Square::H2).shift(Direction::East),
            Bitboard(0)
        );
    }

    #[test]
    fn from_square() {
        assert_eq!(
            Bitboard::from_square(Square::B1),
            Bitboard(0b00000000_00000000_00000000_00000000_00000000_00000000_00000000_01000000)
        );
        assert_eq!(
            Bitboard(0b00000000_00000000_00000000_00000000_00000000_00000000_00000000_01000000),
            bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b01000000
            )
        );

        assert_eq!(
            Bitboard::from_square(Square::E4),
            bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00001000
                0b00000000
                0b00000000
                0b00000000
            )
        );
    }
}
