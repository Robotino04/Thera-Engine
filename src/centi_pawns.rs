use std::{
    cmp::Ordering,
    iter::Sum,
    num::NonZeroI32,
    ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign},
};

use crate::piece::Piece;

#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct CentiPawns(pub i32);
impl CentiPawns {
    pub const fn piece_value(piece: Piece) -> CentiPawns {
        match piece {
            Piece::Pawn => CentiPawns(100),
            Piece::Knight => CentiPawns(320),
            Piece::Bishop => CentiPawns(330),
            Piece::Rook => CentiPawns(500),
            Piece::Queen => CentiPawns(900),
            Piece::King => CentiPawns(0), // ignore the king for most purposes
        }
    }
    pub const MAX_MATERIAL: CentiPawns = CentiPawns(
        2 * (8 * Self::piece_value(Piece::Pawn).0
            + 2 * Self::piece_value(Piece::Knight).0
            + 2 * Self::piece_value(Piece::Bishop).0
            + 2 * Self::piece_value(Piece::Rook).0
            + Self::piece_value(Piece::Queen).0
            + Self::piece_value(Piece::King).0),
    );
}

impl Mul<i32> for CentiPawns {
    type Output = Self;
    fn mul(self, rhs: i32) -> Self::Output {
        Self(self.0 * rhs)
    }
}
impl Mul<CentiPawns> for i32 {
    type Output = CentiPawns;
    fn mul(self, rhs: CentiPawns) -> Self::Output {
        CentiPawns(self * rhs.0)
    }
}
impl MulAssign<i32> for CentiPawns {
    fn mul_assign(&mut self, rhs: i32) {
        *self = *self * rhs
    }
}
impl Add for CentiPawns {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self(self.0 + rhs.0)
    }
}
impl AddAssign for CentiPawns {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs
    }
}
impl Sub for CentiPawns {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self(self.0 - rhs.0)
    }
}
impl SubAssign for CentiPawns {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs
    }
}
impl Sum for CentiPawns {
    fn sum<I: Iterator<Item = Self>>(iter: I) -> Self {
        let mut sum = CentiPawns(0);
        for x in iter {
            sum += x;
        }
        sum
    }
}
impl Neg for CentiPawns {
    type Output = Self;
    fn neg(self) -> Self::Output {
        self * -1
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Evaluation {
    Win(u32),
    Loss(u32),
    CentiPawns(CentiPawns),
}
impl Evaluation {
    pub const MIN: Self = Self::Loss(0); // immediate loss
    pub const MAX: Self = Self::Win(0); // immediate win
    pub const DRAW: Self = Self::CentiPawns(CentiPawns(0));

    pub fn to_uci(self) -> String {
        match self {
            Evaluation::Win(depth) => format!("mate {}", depth.div_ceil(2)),
            Evaluation::Loss(depth) => format!("mate -{}", depth.div_ceil(2)),
            Evaluation::CentiPawns(CentiPawns(centi_pawns)) => format!("cp {centi_pawns}"),
        }
    }

    pub fn next_best(&self) -> Self {
        match self {
            Evaluation::Win(x) => Evaluation::Win(x.saturating_sub(1)),
            Evaluation::Loss(x) => Evaluation::Loss(*x + 1),
            Evaluation::CentiPawns(cp) => Evaluation::CentiPawns(*cp + CentiPawns(1)),
        }
    }
    pub fn next_worst(&self) -> Self {
        match self {
            Evaluation::Win(x) => Evaluation::Win(*x + 1),
            Evaluation::Loss(x) => Evaluation::Loss(x.saturating_sub(1)),
            Evaluation::CentiPawns(cp) => Evaluation::CentiPawns(*cp - CentiPawns(1)),
        }
    }
}

impl Mul<NonZeroI32> for Evaluation {
    type Output = Self;

    fn mul(self, rhs: NonZeroI32) -> Self::Output {
        match (self, rhs.get().cmp(&0)) {
            (_, Ordering::Equal) => unreachable!("rhs is non-zero"),

            (Self::Win(x), Ordering::Less) => Self::Loss(x),
            (Self::Win(x), Ordering::Greater) => Self::Win(x),
            (Self::Loss(x), Ordering::Less) => Self::Win(x),
            (Self::Loss(x), Ordering::Greater) => Self::Loss(x),
            (Self::CentiPawns(cp), _) => Self::CentiPawns(cp * rhs.get()),
        }
    }
}

impl Mul<Evaluation> for NonZeroI32 {
    type Output = Evaluation;

    fn mul(self, rhs: Evaluation) -> Self::Output {
        rhs * self
    }
}
impl Mul<i32> for Evaluation {
    type Output = Self;

    fn mul(self, rhs: i32) -> Self::Output {
        self * NonZeroI32::new(rhs).expect("Evaluations should never be multiplied by zero")
    }
}
impl Mul<Evaluation> for i32 {
    type Output = Evaluation;

    fn mul(self, rhs: Evaluation) -> Self::Output {
        rhs * self
    }
}

impl Neg for Evaluation {
    type Output = Self;

    fn neg(self) -> Self::Output {
        self * -1
    }
}

impl Ord for Evaluation {
    fn cmp(&self, other: &Self) -> Ordering {
        match (self, other) {
            (Self::Win(depth1), Self::Win(depth2)) => depth2.cmp(depth1), // winning earlier is better
            (Self::Win(_depth), Self::CentiPawns(_)) => Ordering::Greater,
            (Self::Win(_depth), Self::Loss(_)) => Ordering::Greater,

            (Self::Loss(_depth), Self::Win(_)) => Ordering::Less,
            (Self::Loss(_depth), Self::CentiPawns(_)) => Ordering::Less,
            (Self::Loss(depth1), Self::Loss(depth2)) => depth1.cmp(depth2), // losing later is better

            (Self::CentiPawns(_cp), Self::Win(_)) => Ordering::Less,
            (Self::CentiPawns(cp1), Self::CentiPawns(cp2)) => cp1.cmp(cp2),
            (Self::CentiPawns(_cp), Self::Loss(_)) => Ordering::Greater,
        }
    }
}
impl PartialOrd for Evaluation {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

#[cfg(test)]
mod test {
    use crate::centi_pawns::{CentiPawns, Evaluation};

    #[test]
    fn eval_ord() {
        assert!(
            Evaluation::Win(10) < Evaluation::Win(0),
            "Earlier wins should be better"
        );
        assert!(
            Evaluation::Win(10000) > Evaluation::Loss(0),
            "Wins should always be better than a loss"
        );
        assert!(
            Evaluation::Win(0) > Evaluation::Loss(10000),
            "Wins should always be better than a loss, even late ones"
        );
        assert!(
            Evaluation::Win(34) > Evaluation::CentiPawns(CentiPawns(4)),
            "Wins should always be better than a normal evaluation"
        );
        assert!(
            Evaluation::Loss(34) < Evaluation::CentiPawns(CentiPawns(4)),
            "Losses should always be worse than a normal evaluation"
        );
    }
}
