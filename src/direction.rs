use crate::typed_array::{TypedArray, TypedIndex};

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
