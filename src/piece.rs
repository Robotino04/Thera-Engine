use itertools::Itertools;

use crate::typed_array::{TypedArray, TypedIndex};

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

    pub fn to_algebraic(self) -> &'static str {
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
