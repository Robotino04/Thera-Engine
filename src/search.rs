use std::{sync::LazyLock, time::Instant};

use itertools::Itertools;
use time::{Duration, ext::InstantExt};

use crate::{
    alpha_beta_window::{AlphaBetaWindow, NodeEvalSummary, WindowUpdate},
    board::Board,
    centi_pawns::{CentiPawns, Evaluation},
    color::Color,
    move_generator::{Move, MoveGenerator},
    piece::{ByPiece, ByPieceTable, Piece},
    square::{BySquare, Square},
    transposition_table::{TranspositionEntry, TranspositionTable},
};

// https://www.chessprogramming.org/Simplified_Evaluation_Function
static PIECE_SQUARE_TABLE: LazyLock<ByPiece<BySquare<CentiPawns>>> = LazyLock::new(|| {
    #[rustfmt::skip]
    let pawn = [
         0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-30,-30, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0,
    ];

    #[rustfmt::skip]
    let knight = [
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-35,-30,-30,-30,-30,-35,-50,
    ];
    #[rustfmt::skip]
    let bishop = [
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20,
    ];
    #[rustfmt::skip]
    let rook = [
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         0,  0,  0,  5,  5,  0,  0,  0,
    ];
    #[rustfmt::skip]
    let queen = [
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20,
    ];
    #[rustfmt::skip]
    let king = [
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20,
    ];

    fn transform(values: [i32; Square::COUNT]) -> BySquare<CentiPawns> {
        BySquare::from_readable(values.into_iter().map(CentiPawns).collect_array().unwrap())
    }

    ByPiece::from_table(ByPieceTable {
        pawn: transform(pawn),
        bishop: transform(bishop),
        knight: transform(knight),
        rook: transform(rook),
        queen: transform(queen),
        king: transform(king),
    })
});

fn eval_piece_placement(board: &Board, color: Color) -> CentiPawns {
    let mut eval = CentiPawns(0);

    for piece in Piece::ALL {
        for square in board.get_piece_bitboard(piece, color).squares() {
            eval += PIECE_SQUARE_TABLE[piece][match color {
                Color::White => square,
                Color::Black => square.flipped(),
            }];
        }
    }

    eval
}

fn eval_material(board: &Board, color: Color) -> CentiPawns {
    let mut material = CentiPawns(0);

    for piece in Piece::ALL {
        material += board.get_piece_bitboard(piece, color).count_ones() as i32
            * CentiPawns::piece_value(piece);
    }

    material
}

fn eval_endgame(
    board: &Board,
    color: Color,
    my_material: CentiPawns,
    their_material: CentiPawns,
) -> CentiPawns {
    let me = color;
    let they = color.opposite();

    let material_advantage = my_material - their_material;

    let their_pawns = board.pawns(they).count_ones() as i32 * CentiPawns::piece_value(Piece::Pawn);

    if material_advantage > 2 * CentiPawns::piece_value(Piece::Pawn) {
        let endgame_material = 2 * CentiPawns::piece_value(Piece::Rook)
            + 2 * CentiPawns::piece_value(Piece::Knight)
            + 2 * CentiPawns::piece_value(Piece::Bishop)
            + 1 * CentiPawns::piece_value(Piece::Queen);

        let endgame_factor = 1.0
            - ((their_material - their_pawns).0 as f32 / endgame_material.0 as f32).clamp(0.0, 1.0);

        let my_king = board.king(me).first_piece_square().unwrap();
        let their_king = board.king(they).first_piece_square().unwrap();

        let close_king_advantage =
            (14 - my_king.manhattan_distance(their_king) as i32) * CentiPawns(25);

        let opponent_center_distance = ((their_king.column() as i32 * 2 - 7).abs()
            + (their_king.row() as i32 * 2 - 7).abs())
            / 2;

        let opponent_in_corner_advantage = opponent_center_distance * CentiPawns(35);

        let endgame_eval = close_king_advantage + opponent_in_corner_advantage;

        CentiPawns((endgame_eval.0 as f32 * endgame_factor) as i32)
    } else {
        CentiPawns(0)
    }
}

fn static_eval(board: &Board) -> Evaluation {
    let me = board.color_to_move();
    let they = board.color_to_move().opposite();

    let my_material = eval_material(board, me);
    let their_material = eval_material(board, they);
    let material_advantage = my_material - their_material;

    let my_placement = eval_piece_placement(board, me);
    let their_placement = eval_piece_placement(board, they);
    let placement_advantage = my_placement - their_placement;

    let my_endgame = eval_endgame(board, me, my_material, their_material);
    let their_endgame = eval_endgame(board, they, their_material, my_material);
    let endgame_advantage = my_endgame - their_endgame;

    Evaluation::CentiPawns(material_advantage + placement_advantage + endgame_advantage)
}

/// This relies on derive(Ord), which always treats later entries as greater.
/// But sort is ascending by default so we have to reverse it as well.
#[derive(Debug, Clone, Copy, Hash, PartialEq, Eq, PartialOrd, Ord)]
enum MoveOrdering {
    QuietMove(CentiPawns),
    Castle,
    Capture(CentiPawns),
    Promotion(CentiPawns),
    PrincipalVariation,
}

impl MoveOrdering {
    fn order_moves(
        moves: impl IntoIterator<Item = Move>,
        board: &Board,
        movegen: &MoveGenerator,
        transposition_table: &TranspositionTable,
    ) -> impl Iterator<Item = Move> {
        moves
            .into_iter()
            .sorted_unstable_by_key(|m| Self::score_move(m, board, movegen, transposition_table))
            .rev()
    }

    fn score_move(
        m: &Move,
        board: &Board,
        movegen: &MoveGenerator,
        transposition_table: &TranspositionTable,
    ) -> MoveOrdering {
        if let Some(entry) = transposition_table.get(board)
            && entry.best_move.is_some_and(|m2| *m == m2)
        {
            return MoveOrdering::PrincipalVariation;
        }

        match m {
            Move::Normal {
                captured_piece,
                moved_piece,
                to_square,
                ..
            } => {
                match (
                    captured_piece,
                    (movegen.attacked_squares() & *to_square).is_empty(),
                ) {
                    (None, true) => MoveOrdering::QuietMove(CentiPawns(0)),
                    (None, false) => {
                        MoveOrdering::QuietMove(-CentiPawns::piece_value(*moved_piece))
                    }
                    (Some(captured_piece), true) => {
                        MoveOrdering::Capture(CentiPawns::piece_value(*captured_piece))
                    }
                    (Some(captured_piece), false) => MoveOrdering::Capture(
                        CentiPawns::piece_value(*captured_piece)
                            - CentiPawns::piece_value(*moved_piece),
                    ),
                }
            }
            Move::DoublePawn { to_square, .. } => {
                if (movegen.attacked_squares() & *to_square).is_empty() {
                    MoveOrdering::QuietMove(CentiPawns(0))
                } else {
                    MoveOrdering::QuietMove(-CentiPawns::piece_value(Piece::Pawn))
                }
            }
            Move::EnPassant { to_square, .. } => {
                if (movegen.attacked_squares() & *to_square).is_empty() {
                    MoveOrdering::Capture(CentiPawns::piece_value(Piece::Pawn))
                } else {
                    MoveOrdering::Capture(CentiPawns(0))
                }
            }
            Move::Castle { .. } => MoveOrdering::Castle,
            Move::Promotion {
                promotion_piece,
                captured_piece,
                to_square,
                ..
            } => {
                if (movegen.attacked_squares() & *to_square).is_empty() {
                    MoveOrdering::Promotion(
                        CentiPawns::piece_value(*promotion_piece)
                            - CentiPawns::piece_value(Piece::Pawn)
                            + captured_piece
                                .map(CentiPawns::piece_value)
                                .unwrap_or_default(),
                    )
                } else {
                    MoveOrdering::Promotion(
                        -CentiPawns::piece_value(Piece::Pawn)
                            + captured_piece
                                .map(CentiPawns::piece_value)
                                .unwrap_or_default(),
                    )
                }
            }
        }
    }
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
) -> Result<NodeEvalSummary, SearchExit> {
    if should_exit() {
        return Err(SearchExit::Cancelled);
    }

    if board.is_draw_50() || board.is_draw_repetition() {
        return Ok(window.set_draw());
    }

    let SearchStats {
        nodes_searched: prev_nodes_searched,
        nodes_searched_quiescence: _,
        transposition_hits: _,
        cached_nodes: _,
        first_move_prune: _,
    } = *stats;

    if let Some(entry) = transposition_table.get_if_improves(board, depth_left, &window) {
        stats.cached_nodes.hits += entry.subnodes;
        stats.transposition_hits.hit();
        return Ok(window.set_exact(entry.eval, entry.best_move));
    } else {
        stats.transposition_hits.miss();
        stats.cached_nodes.miss();
    }

    stats.nodes_searched += 1;

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_all_moves(board);
    if moves.is_empty() {
        let out = if movegen.is_check() {
            window.set_loss()
        } else {
            window.set_draw()
        };

        transposition_table.insert(
            board,
            depth_left,
            out,
            stats.nodes_searched - prev_nodes_searched,
        );
        return Ok(out);
    } else {
        // only call static eval if there are moves. otherwise, we can store the draw or loss in the TT
        if depth_left == 0 {
            let quiescence_out = quiescence_search(
                board,
                window.clone(),
                stats,
                transposition_table,
                should_exit,
            )?;

            transposition_table.insert(
                board,
                depth_left,
                quiescence_out,
                stats.nodes_searched - prev_nodes_searched,
            );
            return Ok(quiescence_out);
        }
    }

    let moves =
        MoveOrdering::order_moves(moves, board, &movegen, transposition_table).collect_vec();

    for (i, m) in moves.into_iter().enumerate() {
        let eval = -search(
            &mut board.with_move(m),
            depth_left - 1,
            window.next_depth(),
            stats,
            should_exit,
            transposition_table,
        )?
        .eval;

        match window.update(eval, Some(m)) {
            WindowUpdate::NewBest | WindowUpdate::NoImprovement => {
                if i == 0 {
                    stats.first_move_prune.miss()
                }
            }
            WindowUpdate::Prune => {
                if i == 0 {
                    stats.first_move_prune.hit()
                }
                break;
            }
        }
    }

    let res = window.finalize();

    transposition_table.insert(
        board,
        depth_left,
        res,
        stats.nodes_searched - prev_nodes_searched,
    );

    Ok(res)
}

fn quiescence_search(
    board: &mut Board,
    mut window: AlphaBetaWindow,
    stats: &mut SearchStats,
    transposition_table: &TranspositionTable,
    should_exit: &impl Fn() -> bool,
) -> Result<NodeEvalSummary, SearchExit> {
    if should_exit() {
        return Err(SearchExit::Cancelled);
    }

    if board.is_draw_50() || board.is_draw_repetition() {
        return Ok(window.set_draw());
    }

    stats.nodes_searched_quiescence += 1;

    let movegen = MoveGenerator::with_attacks(board);
    let captures = movegen.generate_captures(board);

    if captures.is_empty() {
        // no captures
        let all_moves = movegen.generate_all_moves(board);
        if all_moves.is_empty() {
            // and also no other moves => draw or checkmate
            if movegen.is_check() {
                return Ok(window.set_loss());
            } else {
                return Ok(window.set_draw());
            }
        } else {
            // but non-captures => quiet position
            return Ok(window.set_exact(static_eval(board), None));
        }
    }

    match window.update(static_eval(board), None) {
        WindowUpdate::NoImprovement => {}
        WindowUpdate::NewBest => {}
        WindowUpdate::Prune => return Ok(window.finalize()),
    }

    let captures =
        MoveOrdering::order_moves(captures, board, &movegen, transposition_table).collect_vec();

    for (i, m) in captures.into_iter().enumerate() {
        let eval = -quiescence_search(
            &mut board.with_move(m),
            window.next_depth(),
            stats,
            transposition_table,
            should_exit,
        )?
        .eval;

        match window.update(eval, Some(m)) {
            WindowUpdate::NewBest | WindowUpdate::NoImprovement => {
                if i == 0 {
                    stats.first_move_prune.miss()
                }
            }
            WindowUpdate::Prune => {
                if i == 0 {
                    stats.first_move_prune.hit()
                }
                break;
            }
        }
    }

    Ok(window.finalize())
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
pub struct EventTracker {
    pub hits: u64,
    pub misses: u64,
}

impl EventTracker {
    pub fn as_ratio(&self) -> f32 {
        self.hits as f32 / (self.hits + self.misses) as f32
    }

    pub fn hit(&mut self) {
        self.hits += 1;
    }
    pub fn miss(&mut self) {
        self.misses += 1;
    }
}

#[derive(Copy, Clone, Debug, Default, PartialEq)]
pub struct SearchStats {
    pub nodes_searched: u64,
    pub nodes_searched_quiescence: u64,
    pub transposition_hits: EventTracker,
    pub cached_nodes: EventTracker,
    pub first_move_prune: EventTracker,
}

pub struct DepthSummary {
    pub pv: Vec<Move>,
    pub eval: Evaluation,
    pub depth: u32,
    pub time_taken: Duration,
    pub stats: SearchStats,
    pub hash_percentage: f32,
}

fn extract_pv(
    board: &mut Board,
    transposition_table: &TranspositionTable,
    mut depth_left: u32,
) -> Vec<Move> {
    let mut pv = Vec::new();
    while let Some(TranspositionEntry {
        best_move: Some(best_move),
        ..
    }) = transposition_table.get(board)
        && depth_left != 0
    {
        pv.push(best_move);
        board.make_move_unguarded(best_move);
        depth_left -= 1;
    }
    for m in pv.iter().rev() {
        let undone_move = board.try_undo_move();
        assert_eq!(*m, undone_move.unwrap());
    }
    pv
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
    let moves =
        MoveOrdering::order_moves(moves, board, &movegen, transposition_table).collect_vec();

    let Some(&first_move) = moves.first() else {
        return Err(RootSearchExit::NoMove);
    };

    let mut search_stats = SearchStats::default();

    let mut prev_best_move = first_move;
    'search: for depth in 1..options.depth.unwrap_or(256) {
        if should_exit() {
            break 'search;
        }

        let depth_start = Instant::now();

        let search_result = search(
            board,
            depth,
            AlphaBetaWindow::default(),
            &mut search_stats,
            &should_exit,
            transposition_table,
        );

        let out = match search_result {
            Ok(out) => out,
            Err(SearchExit::Cancelled) => break 'search,
        };

        prev_best_move = out.best_move.unwrap_or(prev_best_move);

        on_depth_finished(DepthSummary {
            pv: extract_pv(board, transposition_table, depth),
            eval: out.eval,
            depth,
            time_taken: Instant::now().signed_duration_since(depth_start),
            stats: search_stats,
            hash_percentage: transposition_table.used_slots() as f32
                / transposition_table.capacity() as f32,
        });
    }

    Ok(prev_best_move)
}
