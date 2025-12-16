use crate::typed_array::{TypedArray, TypedIndex};

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
