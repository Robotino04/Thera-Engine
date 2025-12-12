use std::time::Instant;

use time::{Duration, ext::InstantExt};

use crate::{
    alpha_beta_window::{AlphaBetaWindow, WindowUpdate},
    board::Board,
    centi_pawns::{CentiPawns, Evaluation},
    move_generator::MoveGenerator,
    piece::{Color, Move, Piece},
    transposition_table::TranspositionTable,
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
    mut window: AlphaBetaWindow,
    stats: &mut SearchStats,
    should_exit: &impl Fn() -> bool,
    transposition_table: &mut TranspositionTable,
) -> Result<Evaluation, SearchExit> {
    if should_exit() {
        return Err(SearchExit::Cancelled);
    }

    if board.is_draw_50() || board.is_draw_repetition() {
        return Ok(Evaluation::DRAW);
    }

    let SearchStats {
        nodes_searched: prev_nodes_searched,
        nodes_searched_quiescence: _,
        transposition_hits: _,
        cached_nodes: _,
    } = *stats;

    if let Some(entry) = transposition_table.get(board, depth_left, &window) {
        stats.cached_nodes += entry.subnodes;
        stats.transposition_hits += 1;
        return Ok(entry.eval);
    }

    stats.nodes_searched += 1;

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_all_moves(board);
    if moves.is_empty() {
        let eval = if movegen.is_check() {
            Evaluation::Loss(window.plies())
        } else {
            Evaluation::DRAW
        };

        transposition_table.insert(
            board,
            depth_left,
            window.set_exact(eval),
            stats.nodes_searched - prev_nodes_searched,
        );
        return Ok(eval);
    } else {
        // only call static eval if there are moves. otherwise, we can store the draw or loss in the TT
        if depth_left == 0 {
            return quiescence_search(board, window, stats, should_exit);
        }
    }

    for m in moves {
        let eval = -search(
            &mut board.with_move(m),
            depth_left - 1,
            window.next_depth(),
            stats,
            should_exit,
            transposition_table,
        )?;

        match window.update(eval) {
            WindowUpdate::NoImprovement => {}
            WindowUpdate::NewBest => {}
            WindowUpdate::Prune => break,
        }
    }

    let res = window.finalize();

    transposition_table.insert(
        board,
        depth_left,
        res,
        stats.nodes_searched - prev_nodes_searched,
    );

    Ok(res.eval)
}

fn quiescence_search(
    board: &mut Board,
    mut window: AlphaBetaWindow,
    stats: &mut SearchStats,
    should_exit: &impl Fn() -> bool,
) -> Result<Evaluation, SearchExit> {
    if should_exit() {
        return Err(SearchExit::Cancelled);
    }

    if board.is_draw_50() || board.is_draw_repetition() {
        return Ok(Evaluation::DRAW);
    }

    stats.nodes_searched_quiescence += 1;

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_captures(board);
    if moves.is_empty() {
        let moves = movegen.generate_all_moves(board);
        // no captures
        if moves.is_empty() {
            // and also no other moves => draw or checkmate
            if movegen.is_check() {
                return Ok(Evaluation::Loss(window.plies()));
            } else {
                return Ok(Evaluation::DRAW);
            }
        } else {
            // but non-captures => quiet position
            return Ok(static_eval(board));
        }
    }
    match window.update(static_eval(board)) {
        WindowUpdate::NoImprovement => {}
        WindowUpdate::NewBest => {}
        WindowUpdate::Prune => return Ok(window.finalize().eval),
    }

    for m in moves {
        let eval = -quiescence_search(
            &mut board.with_move(m),
            window.next_depth(),
            stats,
            should_exit,
        )?;

        match window.update(eval) {
            WindowUpdate::NoImprovement => {}
            WindowUpdate::NewBest => {}
            WindowUpdate::Prune => break,
        }
    }

    Ok(window.finalize().eval)
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

#[derive(Copy, Clone, Debug, Default, PartialEq)]
pub struct SearchStats {
    pub nodes_searched: u64,
    pub nodes_searched_quiescence: u64,
    pub transposition_hits: u64,
    pub cached_nodes: u64,
}

pub struct DepthSummary {
    pub best_move: Move,
    pub eval: Evaluation,
    pub depth: u32,
    pub time_taken: Duration,
    pub stats: SearchStats,
    pub hash_percentage: f32,
}

pub fn search_root(
    board: &mut Board,
    options: SearchOptions,
    on_depth_finished: impl Fn(DepthSummary),
    transposition_table: &mut TranspositionTable,
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

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_all_moves(board);

    if moves.is_empty() {
        return Err(RootSearchExit::NoMove);
    }

    let mut search_stats = SearchStats::default();

    let mut prev_best_move = *moves.first().unwrap();
    'search: for depth in 1..options.depth.unwrap_or(u32::MAX) {
        let mut best = *moves.first().unwrap();
        let depth_start = Instant::now();

        let mut window = AlphaBetaWindow::default();

        for &m in &moves {
            let eval = search(
                &mut board.with_move(m),
                depth - 1,
                window.next_depth(),
                &mut search_stats,
                &should_exit,
                transposition_table,
            );

            let eval = match eval {
                Ok(eval) => -eval,
                Err(SearchExit::Cancelled) => break 'search,
            };

            match window.update(eval) {
                WindowUpdate::NewBest => {
                    best = m;
                }
                WindowUpdate::NoImprovement => {}
                WindowUpdate::Prune => break,
            }
        }

        on_depth_finished(DepthSummary {
            best_move: best,
            eval: window.finalize().eval,
            depth,
            time_taken: Instant::now().signed_duration_since(depth_start),
            stats: search_stats,
            hash_percentage: transposition_table.used_slots() as f32
                / transposition_table.capacity() as f32,
        });

        prev_best_move = best;
    }

    Ok(prev_best_move)
}
