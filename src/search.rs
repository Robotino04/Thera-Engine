use std::time::Instant;

use time::{Duration, ext::InstantExt};

use crate::{
    board::Board,
    centi_pawns::{CentiPawns, Evaluation},
    move_generator::MoveGenerator,
    piece::{Color, Move, Piece},
};

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
                    * CentiPawns::piece_value(piece)
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

    let mut best_score = Evaluation::MIN;
    let movegen = MoveGenerator::<true>::with_attacks(board);
    let moves = movegen.generate_all_moves(board);
    if moves.is_empty() {
        if movegen.is_check() {
            best_score = Evaluation::Loss(plies);
        } else {
            best_score = Evaluation::DRAW;
        }
    }

    if depth_left == 0 {
        if moves.is_empty() {
            return Ok(best_score);
        } else {
            return Ok(static_eval(board));
        }
    }

    {
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

    if moves.is_empty() {
        return Err(RootSearchExit::NoMove);
    }

    let mut search_stats = SearchStats::default();

    let mut prev_best_move = (Evaluation::MIN, *moves.first().unwrap());
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
                time_taken: Instant::now().signed_duration_since(depth_start),
                stats: search_stats,
            });
        }

        if let Some(best_move) = best_move {
            prev_best_move = best_move;
        }
    }

    Ok(prev_best_move.1)
}
