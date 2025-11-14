use itertools::Itertools;

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
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
}
impl Square {
    #[rustfmt::skip]
    pub const ALL: [Square; 64] = {
        use Square::*;
        [
            A8, B8, C8, D8, E8, F8, G8, H8,
            A7, B7, C7, D7, E7, F7, G7, H7,
            A6, B6, C6, D6, E6, F6, G6, H6,
            A5, B5, C5, D5, E5, F5, G5, H5,
            A4, B4, C4, D4, E4, F4, G4, H4,
            A3, B3, C3, D3, E3, F3, G3, H3,
            A2, B2, C2, D2, E2, F2, G2, H2,
            A1, B1, C1, D1, E1, F1, G1, H1,
        ]
    };

    pub fn new(index: u8) -> Option<Self> {
        Self::ALL.get(index as usize).copied()
    }

    pub fn from_xy<T>(x: T, y: T) -> Option<Self>
    where
        T: TryInto<u8>,
    {
        let x = x.try_into().ok()?;
        let y = y.try_into().ok()?;

        if (0..8).contains(&x) && (0..8).contains(&y) {
            Self::ALL.get((y * 8 + x) as usize).copied()
        } else {
            None
        }
    }

    pub fn from_algebraic(enpassant_str: &str) -> Option<Self> {
        let [x_str, y_str] = enpassant_str.chars().collect_array()?;
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
