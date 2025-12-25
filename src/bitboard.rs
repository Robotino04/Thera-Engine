use std::ops::{
    BitAnd, BitAndAssign, BitOr, BitOrAssign, BitXor, BitXorAssign, Deref, DerefMut, Not,
};

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

use crate::{direction::Direction, square::Square};

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

#[derive(Copy, Clone)]
pub struct BitIter(Bitboard);

impl Iterator for BitIter {
    type Item = Bitboard;

    fn next(&mut self) -> Option<Self::Item> {
        self.0.bitscan()
    }
    fn size_hint(&self) -> (usize, Option<usize>) {
        let count = self.0.count_ones() as usize;
        (count, Some(count))
    }
}
impl ExactSizeIterator for BitIter {}
impl DoubleEndedIterator for BitIter {
    fn next_back(&mut self) -> Option<Self::Item> {
        self.0.bitscan_reverse()
    }
}

#[derive(Copy, Clone)]
pub struct IndexIter(Bitboard);

impl Iterator for IndexIter {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        self.0.bitscan_index()
    }
    fn size_hint(&self) -> (usize, Option<usize>) {
        let count = self.0.count_ones() as usize;
        (count, Some(count))
    }
}
impl ExactSizeIterator for IndexIter {}
impl DoubleEndedIterator for IndexIter {
    fn next_back(&mut self) -> Option<Self::Item> {
        self.0.bitscan_reverse_index()
    }
}

#[derive(Copy, Clone)]
pub struct SquareIter(Bitboard);

impl Iterator for SquareIter {
    type Item = Square;

    fn next(&mut self) -> Option<Self::Item> {
        self.0.bitscan_square()
    }
    fn size_hint(&self) -> (usize, Option<usize>) {
        let count = self.0.count_ones() as usize;
        (count, Some(count))
    }
}
impl ExactSizeIterator for SquareIter {}
impl DoubleEndedIterator for SquareIter {
    fn next_back(&mut self) -> Option<Self::Item> {
        self.0.bitscan_reverse_square()
    }
}

impl IntoIterator for Bitboard {
    type Item = Bitboard;
    type IntoIter = BitIter;

    fn into_iter(self) -> Self::IntoIter {
        BitIter(self)
    }
}

impl Bitboard {
    pub const fn from_rows(rows: [u8; 8]) -> Self {
        Bitboard(u64::from_be_bytes(rows))
    }

    pub const fn at(&self, index: Square) -> bool {
        (self.0 >> index as u32) & 0b1 == 1
    }
    pub const fn set(&mut self, index: Square) {
        self.0 |= 1 << index as u32
    }
    pub const fn clear(&mut self, index: Square) {
        self.0 &= !(1 << index as u32)
    }
    pub const fn assign(&mut self, index: Square, value: bool) {
        self.clear(index);
        self.0 |= (value as u64) << (index as u32)
    }

    pub const fn from_square(square: Square) -> Bitboard {
        Bitboard(1 << (square as u32))
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

    pub const fn bits(self) -> BitIter {
        BitIter(self)
    }
    pub const fn indices(self) -> IndexIter {
        IndexIter(self)
    }
    pub const fn squares(self) -> SquareIter {
        SquareIter(self)
    }

    const fn least_significant_bit(&self) -> Option<Bitboard> {
        if self.0 == 0 {
            None
        } else {
            Some(Bitboard(self.0 & self.0.wrapping_neg()))
        }
    }
    const fn least_significant_bit_index(&self) -> Option<u32> {
        if self.0 == 0 {
            None
        } else {
            Some(self.0.trailing_zeros())
        }
    }
    const fn clear_least_significant_bit(&mut self) {
        self.0 &= self.0.wrapping_sub(1);
    }

    const fn most_significant_bit(&self) -> Option<Bitboard> {
        if let Some(index) = self.most_significant_bit_index() {
            Some(Bitboard(1 << index))
        } else {
            None
        }
    }
    const fn most_significant_bit_index(&self) -> Option<u32> {
        if self.0 == 0 {
            None
        } else {
            Some(63 - self.0.leading_zeros())
        }
    }
    const fn clear_most_significant_bit(&mut self) {
        if let Some(mask) = self.most_significant_bit() {
            self.0 &= !mask.0;
        }
    }

    const fn bitscan(&mut self) -> Option<Bitboard> {
        let mask = self.least_significant_bit();
        self.clear_least_significant_bit();
        mask
    }
    const fn bitscan_index(&mut self) -> Option<u32> {
        let index = self.least_significant_bit_index();
        self.clear_least_significant_bit();
        index
    }
    fn bitscan_square(&mut self) -> Option<Square> {
        self.bitscan_index().map(|x| Square::new(x).unwrap())
    }

    const fn bitscan_reverse(&mut self) -> Option<Bitboard> {
        let mask = self.most_significant_bit();
        self.clear_most_significant_bit();
        mask
    }
    const fn bitscan_reverse_index(&mut self) -> Option<u32> {
        let index = self.most_significant_bit_index();
        self.clear_most_significant_bit();
        index
    }
    fn bitscan_reverse_square(&mut self) -> Option<Square> {
        self.bitscan_reverse_index()
            .map(|x| Square::new(x).unwrap())
    }

    pub const fn first_piece(&self) -> Option<Bitboard> {
        self.least_significant_bit()
    }
    pub const fn first_piece_index(&self) -> Option<u32> {
        self.least_significant_bit_index()
    }
    pub fn first_piece_square(&self) -> Option<Square> {
        self.first_piece_index().map(|x| Square::new(x).unwrap())
    }

    /// NOTE: you should always try to use `first_piece` instead if possible
    pub const fn last_piece(&self) -> Option<Bitboard> {
        self.most_significant_bit()
    }
    /// NOTE: you should always try to use `first_piece_index` instead if possible
    pub const fn last_piece_index(&self) -> Option<u32> {
        self.most_significant_bit_index()
    }
    /// NOTE: you should always try to use `first_piece_square` instead if possible
    pub fn last_piece_square(&self) -> Option<Square> {
        self.last_piece_index().map(|x| Square::new(x).unwrap())
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
    use crate::{bitboard::Bitboard, direction::Direction, square::Square};

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
