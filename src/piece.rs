use std::fmt::Display;

use itertools::Itertools;

use crate::{
    bitboard::Bitboard,
    typed_array::{TypedArray, TypedIndex},
};

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

impl TypedIndex for Piece {
    fn typed_index(self) -> usize {
        self as usize
    }
}

pub type ByPiece<T> = TypedArray<T, { Piece::COUNT }, Piece>;

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ByPieceTable<T> {
    pub pawn: T,
    pub bishop: T,
    pub knight: T,
    pub rook: T,
    pub queen: T,
    pub king: T,
}

impl<T> ByPiece<T> {
    pub fn from_table(table: ByPieceTable<T>) -> Self {
        let ByPieceTable {
            pawn,
            bishop,
            knight,
            rook,
            queen,
            king,
        } = table;

        let mut array = ByPiece::new([const { None }; Piece::COUNT]);
        array[Piece::Pawn] = Some(pawn);
        array[Piece::Bishop] = Some(bishop);
        array[Piece::Knight] = Some(knight);
        array[Piece::Rook] = Some(rook);
        array[Piece::Queen] = Some(queen);
        array[Piece::King] = Some(king);

        Self::new(
            array
                .into_inner()
                .into_iter()
                .map(Option::unwrap)
                .collect_array()
                .unwrap(),
        )
    }
}

impl Piece {
    pub const COUNT: usize = 6;
    pub const ALL: [Piece; Self::COUNT] = [
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

    fn to_algebraic(self) -> &'static str {
        match self {
            Piece::Pawn => "p",
            Piece::Bishop => "b",
            Piece::Knight => "n",
            Piece::Rook => "r",
            Piece::Queen => "q",
            Piece::King => "k",
        }
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

impl TypedIndex for Direction {
    fn typed_index(self) -> usize {
        self as usize
    }
}

pub type ByDirection<T> = TypedArray<T, { Direction::COUNT }, Direction>;

impl Direction {
    pub const COUNT: usize = 8;
    pub const ALL: [Direction; Self::COUNT] = [
        Self::North,
        Self::South,
        Self::East,
        Self::West,
        Self::NorthEast,
        Self::NorthWest,
        Self::SouthEast,
        Self::SouthWest,
    ];

    pub const fn index(self) -> usize {
        match self {
            Direction::North => 0,
            Direction::South => 1,
            Direction::East => 2,
            Direction::West => 3,
            Direction::NorthEast => 4,
            Direction::NorthWest => 5,
            Direction::SouthEast => 6,
            Direction::SouthWest => 7,
        }
    }

    pub const fn opposite(self) -> Self {
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
    pub const COUNT: usize = 2;
    pub const ALL: [Color; Self::COUNT] = [Self::White, Self::Black];

    pub fn opposite(&self) -> Color {
        match self {
            Color::White => Color::Black,
            Color::Black => Color::White,
        }
    }
}

impl TypedIndex for Color {
    fn typed_index(self) -> usize {
        self as usize
    }
}

pub type ByColor<T> = TypedArray<T, { Color::COUNT }, Color>;

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
    pub const COUNT: usize = 64;
    #[rustfmt::skip]
    pub const ALL: [Square; Self::COUNT] = {
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
    pub fn to_algebraic(&self) -> &'static str {
        #[rustfmt::skip]
        let strs = [
            "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
            "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
            "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
            "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
            "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
            "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
            "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
            "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
        ];
        strs[*self as usize]
    }

    pub const fn row(&self) -> u8 {
        *self as u8 / 8
    }
    pub const fn column(&self) -> u8 {
        7 - *self as u8 % 8
    }

    pub fn flipped(&self) -> Self {
        Self::from_xy(self.column(), 7 - self.row()).unwrap()
    }
}

impl TypedIndex for Square {
    fn typed_index(self) -> usize {
        self as usize
    }
}

pub type BySquare<T> = TypedArray<T, { Square::COUNT }, Square>;

impl<T> BySquare<T> {
    /// flips the array so you can write it from whites perspective in code
    pub fn from_readable(value: [T; Square::COUNT]) -> Self {
        Self::new(
            value
                .into_iter()
                .chunks(8)
                .into_iter()
                .map(|row| row.collect_array::<8>().unwrap().into_iter().rev())
                .collect_array::<8>()
                .unwrap()
                .into_iter()
                .rev()
                .flatten()
                .collect_array::<{ Square::COUNT }>()
                .unwrap(),
        )
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Move {
    Normal {
        from_square: Bitboard,
        to_square: Bitboard,
        captured_piece: Option<Piece>,
        moved_piece: Piece,
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
        captured_piece: Option<Piece>,
    },
}

impl Display for Move {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.to_algebraic())
    }
}

impl Move {
    pub fn to_algebraic(&self) -> String {
        match self {
            Move::Normal {
                from_square,
                to_square,
                captured_piece: _,
                moved_piece: _,
            }
            | Move::DoublePawn {
                from_square,
                to_square,
            }
            | Move::EnPassant {
                from_square,
                to_square,
            }
            | Move::Castle {
                from_square,
                to_square,
                rook_from_square: _,
                rook_to_square: _,
            } => {
                from_square
                    .first_piece_square()
                    .unwrap()
                    .to_algebraic()
                    .to_string()
                    + to_square.first_piece_square().unwrap().to_algebraic()
            }
            Move::Promotion {
                from_square,
                to_square,
                promotion_piece,
                captured_piece: _,
            } => {
                from_square
                    .first_piece_square()
                    .unwrap()
                    .to_algebraic()
                    .to_string()
                    + to_square.first_piece_square().unwrap().to_algebraic()
                    + promotion_piece.to_algebraic()
            }
        }
    }
}

#[cfg(test)]
mod test {
    use crate::piece::{BySquare, Square};

    #[test]
    fn from_xy_directions() {
        assert_eq!(Square::from_xy(0, 0), Some(Square::A1));
        assert_eq!(Square::from_xy(0, 7), Some(Square::A8));
        assert_eq!(Square::from_xy(7, 0), Some(Square::H1));
        assert_eq!(Square::from_xy(7, 7), Some(Square::H8));

        assert_eq!(Square::from_xy(4, 1), Some(Square::E2));
        assert_eq!(Square::from_xy(1, 6), Some(Square::B7));
    }

    #[test]
    fn flipping() {
        assert_eq!(Square::A5.flipped(), Square::A4);
        assert_eq!(Square::E1.flipped(), Square::E8);
        assert_eq!(Square::F7.flipped(), Square::F2);
        assert_eq!(Square::B3.flipped(), Square::B6);
    }

    #[test]
    fn from_square_reversible() {
        let test = |a: Square| {
            assert_eq!(Square::from_xy(a.column(), a.row()).unwrap(), a);
        };
        test(Square::A1);
        test(Square::B7);
        test(Square::E3);
    }

    #[test]
    fn by_square_from_readable() {
        #[rustfmt::skip]
        let value = [
            0, 0, 0, 0,  0, 0, 0,  0,
            0, 0, 0, 0,  0, 0, 0,  0,
            0, 0, 0, 0,  0, 0, 0, -3,
            0, 9, 0, 0,  0, 0, 0,  0,
            0, 0, 0, 0,  0, 0, 0,  0,
            0, 0, 0, 0, 75, 0, 0,  0,
            0, 0, 0, 0,  0, 0, 0,  0,
            0, 0, 0, 0,  0, 0, 0,  0,
        ];

        println!("{:#?}", BySquare::from_readable(value));
        assert_eq!(BySquare::from_readable(value)[Square::E3], 75);
        assert_eq!(BySquare::from_readable(value)[Square::H6], -3);
        assert_eq!(BySquare::from_readable(value)[Square::B5], 9);
    }

    #[test]
    fn by_square_from_readable_whole_board() {
        use Square as S;

        #[rustfmt::skip]
        let value = [
            S::A8, S::B8, S::C8, S::D8, S::E8, S::F8, S::G8, S::H8, 
            S::A7, S::B7, S::C7, S::D7, S::E7, S::F7, S::G7, S::H7, 
            S::A6, S::B6, S::C6, S::D6, S::E6, S::F6, S::G6, S::H6, 
            S::A5, S::B5, S::C5, S::D5, S::E5, S::F5, S::G5, S::H5, 
            S::A4, S::B4, S::C4, S::D4, S::E4, S::F4, S::G4, S::H4, 
            S::A3, S::B3, S::C3, S::D3, S::E3, S::F3, S::G3, S::H3, 
            S::A2, S::B2, S::C2, S::D2, S::E2, S::F2, S::G2, S::H2, 
            S::A1, S::B1, S::C1, S::D1, S::E1, S::F1, S::G1, S::H1, 
        ];

        let flipped = BySquare::from_readable(value);
        for square in Square::ALL {
            assert_eq!(flipped[square], square);
        }
    }
}
