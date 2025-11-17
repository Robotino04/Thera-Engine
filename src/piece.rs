use itertools::Itertools;

use crate::bitboard::Bitboard;

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum Piece {
    Pawn,
    Bishop,
    Knight,
    Rook,
    Queen,
    King,
}

impl Piece {
    pub const ALL: [Piece; 6] = [
        Self::Pawn,
        Self::Bishop,
        Self::Knight,
        Self::Rook,
        Self::Queen,
        Self::King,
    ];

    pub fn from_algebraic(c: char) -> Option<Self> {
        Some(match c.to_ascii_lowercase() {
            'p' => Piece::Pawn,
            'b' => Piece::Bishop,
            'n' => Piece::Knight,
            'r' => Piece::Rook,
            'q' => Piece::Queen,
            'k' => Piece::King,
            _ => return None,
        })
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
#[repr(i8)]
pub enum Direction {
    North = 8,
    South = -8,
    East = -1,
    West = 1,

    NorthEast = Self::North as i8 + Self::East as i8,
    NorthWest = Self::North as i8 + Self::West as i8,
    SouthEast = Self::South as i8 + Self::East as i8,
    SouthWest = Self::South as i8 + Self::West as i8,
}

impl Direction {
    pub fn opposite(self) -> Self {
        match self {
            Direction::North => Direction::South,
            Direction::South => Direction::North,
            Direction::East => Direction::West,
            Direction::West => Direction::East,
            Direction::NorthEast => Direction::SouthWest,
            Direction::NorthWest => Direction::SouthEast,
            Direction::SouthEast => Direction::NorthWest,
            Direction::SouthWest => Direction::NorthEast,
        }
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum Color {
    White,
    Black,
}

impl Color {
    pub const ALL: [Color; 2] = [Self::White, Self::Black];

    pub fn opposite(&self) -> Color {
        match self {
            Color::White => Color::Black,
            Color::Black => Color::White,
        }
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct Rank {
    index: u8,
}

#[rustfmt::skip]
#[repr(u8)]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Square {
    H1, G1, F1, E1, D1, C1, B1, A1,
    H2, G2, F2, E2, D2, C2, B2, A2,
    H3, G3, F3, E3, D3, C3, B3, A3,
    H4, G4, F4, E4, D4, C4, B4, A4,
    H5, G5, F5, E5, D5, C5, B5, A5,
    H6, G6, F6, E6, D6, C6, B6, A6,
    H7, G7, F7, E7, D7, C7, B7, A7,
    H8, G8, F8, E8, D8, C8, B8, A8,
}

impl Square {
    #[rustfmt::skip]
    pub const ALL: [Square; 64] = {
        use Square::*;
        [
            H1, G1, F1, E1, D1, C1, B1, A1,
            H2, G2, F2, E2, D2, C2, B2, A2,
            H3, G3, F3, E3, D3, C3, B3, A3,
            H4, G4, F4, E4, D4, C4, B4, A4,
            H5, G5, F5, E5, D5, C5, B5, A5,
            H6, G6, F6, E6, D6, C6, B6, A6,
            H7, G7, F7, E7, D7, C7, B7, A7,
            H8, G8, F8, E8, D8, C8, B8, A8,
        ]
    };

    pub fn new(index: u8) -> Option<Self> {
        Self::ALL.get(index as usize).copied()
    }

    /// 0, 0 is the bottom left
    pub fn from_xy<T>(x: T, y: T) -> Option<Self>
    where
        T: TryInto<u8>,
    {
        let x = x.try_into().ok()?;
        let y = y.try_into().ok()?;

        if (0..8).contains(&x) && (0..8).contains(&y) {
            Self::ALL.get((y * 8 + (7 - x)) as usize).copied()
        } else {
            None
        }
    }

    pub fn from_algebraic(s: &str) -> Option<Self> {
        let [x_str, y_str] = s.chars().collect_array()?;
        let x = match x_str {
            'a' => 0,
            'b' => 1,
            'c' => 2,
            'd' => 3,
            'e' => 4,
            'f' => 5,
            'g' => 6,
            'h' => 7,
            _ => return None,
        };
        let y = match y_str {
            '1' => 0,
            '2' => 1,
            '3' => 2,
            '4' => 3,
            '5' => 4,
            '6' => 5,
            '7' => 6,
            '8' => 7,
            _ => return None,
        };
        Self::from_xy(x, y)
    }

    pub const fn row(&self) -> u8 {
        *self as u8 / 8
    }
    pub const fn column(&self) -> u8 {
        *self as u8 % 8
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Move {
    Normal {
        from_square: Bitboard,
        to_square: Bitboard,
        is_capture: bool,
    },
    DoublePawn {
        from_square: Bitboard,
        to_square: Bitboard,
    },
    EnPassant {
        from_square: Bitboard,
        to_square: Bitboard,
    },
    Castle {
        from_square: Bitboard,
        to_square: Bitboard,
        rook_from_square: Bitboard,
        rook_to_square: Bitboard,
    },
    Promotion {
        from_square: Bitboard,
        to_square: Bitboard,
        promotion_piece: Piece,
        is_capture: bool,
    },
}

impl Move {
    pub fn from_algebraic(s: &str) -> Option<Move> {
        let from_square = Bitboard::from_square(Square::from_algebraic(s.get(0..2)?)?);
        let to_square = Bitboard::from_square(Square::from_algebraic(s.get(2..4)?)?);

        if let Some(c) = s.chars().nth(5) {
            Some(Move::Promotion {
                from_square,
                to_square,
                promotion_piece: match Piece::from_algebraic(c)? {
                    Piece::Pawn | Piece::King => return None,
                    p @ (Piece::Bishop | Piece::Knight | Piece::Rook | Piece::Queen) => p,
                },
                is_capture: false,
            })
        } else {
            Some(Move::Normal {
                from_square,
                to_square,
                is_capture: false,
            })
        }
    }
}

#[cfg(test)]
mod test {
    use crate::piece::Square;

    #[test]
    fn from_xy_directions() {
        assert_eq!(Square::from_xy(0, 0), Some(Square::A1));
        assert_eq!(Square::from_xy(0, 7), Some(Square::A8));
        assert_eq!(Square::from_xy(7, 0), Some(Square::H1));
        assert_eq!(Square::from_xy(7, 7), Some(Square::H8));

        assert_eq!(Square::from_xy(4, 1), Some(Square::E2));
        assert_eq!(Square::from_xy(1, 6), Some(Square::B7));
    }
}
