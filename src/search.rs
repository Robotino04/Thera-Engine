use std::{
    cmp::Ordering,
    iter::Sum,
    num::NonZeroI32,
    ops::{Add, AddAssign, Mul, MulAssign, Neg},
    time::{Duration, Instant},
};

use crate::{
    board::Board,
    move_generator::MoveGenerator,
    piece::{Color, Move, Piece},
};

#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct CentiPawns(pub i32);
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
    const MIN: Self = Self::Loss(0); // immediate loss
    const MAX: Self = Self::Win(0); // immediate win
    const DRAW: Self = Self::CentiPawns(CentiPawns(0));

    pub fn to_uci(self) -> String {
        match self {
            Evaluation::Win(depth) => format!("mate {}", depth.div_ceil(2)),
            Evaluation::Loss(depth) => format!("mate -{}", depth.div_ceil(2)),
            Evaluation::CentiPawns(CentiPawns(centi_pawns)) => format!("cp {centi_pawns}"),
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

fn piece_value(piece: Piece) -> CentiPawns {
    match piece {
        Piece::Pawn => CentiPawns(100),
        Piece::Bishop => CentiPawns(300),
        Piece::Knight => CentiPawns(300),
        Piece::Rook => CentiPawns(500),
        Piece::Queen => CentiPawns(900),
        Piece::King => CentiPawns(20000),
    }
}

fn static_eval(board: &Board) -> Evaluation {
    let player = board.color_to_move();
    let opponent = board.color_to_move().opposite();

    Evaluation::CentiPawns(
        Piece::ALL
            .iter()
            .cloned()
            .map(|piece| {
                (board.get_piece_bitboard(piece, player).count_ones() as i32
                    - board.get_piece_bitboard(piece, opponent).count_ones() as i32)
                    * piece_value(piece)
            })
            .sum(),
    )
}

pub enum SearchExit {
    Cancelled,
}

fn search(
    board: &mut Board,
    depth_left: u32,
    mut alpha: Evaluation,
    beta: Evaluation,
    stats: &mut SearchStats,
    plies: u32,
    should_exit: &impl Fn() -> bool,
) -> Result<Evaluation, SearchExit> {
    if should_exit() {
        return Err(SearchExit::Cancelled);
    }

    stats.nodes_searched += 1;

    if board.is_draw_50() | board.is_draw_repetition() {
        return Ok(Evaluation::DRAW);
    }

    if depth_left == 0 {
        return Ok(static_eval(board));
    }

    let movegen = MoveGenerator::<true>::with_attacks(board);
    let moves = movegen.generate_all_moves(board);

    let mut best_score = Evaluation::MIN;

    if moves.is_empty() {
        if movegen.is_check() {
            best_score = Evaluation::Loss(plies);
        } else {
            best_score = Evaluation::DRAW;
        }
    } else {
        for m in moves {
            let undo = board.make_move(m);
            let eval = -search(
                board,
                depth_left - 1,
                -beta,
                -alpha,
                stats,
                plies + 1,
                should_exit,
            )?;
            board.undo_move(undo);

            if eval > best_score {
                best_score = eval;
            }
            if eval > alpha {
                alpha = eval;
            }
            if alpha >= beta {
                return Ok(alpha);
            }
        }
    }

    Ok(best_score)
}

pub enum RootSearchExit {
    NoMove,
}

pub struct SearchOptions {
    pub depth: Option<u32>,
    pub movetime: Option<Duration>,
    pub wtime: Option<Duration>,
    pub btime: Option<Duration>,
    pub winc: Option<Duration>,
    pub binc: Option<Duration>,
}

#[derive(Copy, Clone, Debug, Hash, Default, PartialEq, Eq)]
pub struct SearchStats {
    pub nodes_searched: u64,
}

pub struct DepthSummary {
    pub best_move: Move,
    pub eval: Evaluation,
    pub depth: u32,
    pub time_taken: Duration,
    pub stats: SearchStats,
}

pub fn search_root(
    board: &mut Board,
    options: SearchOptions,
    on_depth_finished: impl Fn(DepthSummary),
    should_exit: impl Fn() -> bool,
) -> Result<Move, RootSearchExit> {
    let search_start = Instant::now();
    let (my_time, my_inc) = match board.color_to_move() {
        Color::White => (options.wtime, options.winc),
        Color::Black => (options.btime, options.binc),
    };

    let max_search_time = my_time.map(|my_time| my_time / 40 + my_inc.unwrap_or_default());

    let max_search_time = match (options.movetime, max_search_time) {
        (None, None) => None,
        (None, Some(max_search_time)) => Some(max_search_time),
        (Some(movetime), None) => Some(movetime),
        (Some(movetime), Some(max_search_time)) => Some(movetime.min(max_search_time)),
    };

    let search_end_time = max_search_time.map(|max_search_time| search_start + max_search_time);

    let should_exit = || {
        should_exit()
            || search_end_time.is_some_and(|search_end_time| Instant::now() >= search_end_time)
    };

    let movegen = MoveGenerator::<true>::with_attacks(board);
    let moves = movegen.generate_all_moves(board);

    let mut search_stats = SearchStats::default();

    let mut prev_best_move = None;
    'search: for depth in 1..options.depth.unwrap_or(u32::MAX) {
        let mut best_move = None;
        let depth_start = Instant::now();

        for &m in &moves {
            let undo = board.make_move(m);
            let eval = match search(
                board,
                depth - 1,
                Evaluation::MIN,
                Evaluation::MAX,
                &mut search_stats,
                1,
                &should_exit,
            ) {
                Ok(eval) => -eval,
                Err(SearchExit::Cancelled) => break 'search,
            };
            board.undo_move(undo);
            if best_move.is_none_or(|(best_eval, _best_move)| eval > best_eval) {
                best_move = Some((eval, m));
            }
        }

        if let Some((eval, best_move)) = best_move {
            on_depth_finished(DepthSummary {
                best_move,
                eval,
                depth,
                time_taken: Instant::now() - depth_start,
                stats: search_stats,
            });
        }

        prev_best_move = best_move;
    }

    prev_best_move
        .map(|(_best_eval, best_move)| best_move)
        .ok_or(RootSearchExit::NoMove)
}
