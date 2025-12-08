use crate::{board::Board, move_generator::MoveGenerator, piece::Move};

#[derive(Clone, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct PerftMove {
    pub algebraic_move: String,
    pub nodes: usize,
}

#[derive(Clone, Debug, PartialEq, Eq, Default)]
pub struct PerftStatistics {
    pub divide: Vec<PerftMove>,
    pub node_count: usize,
    pub checks: usize,
    pub double_checks: usize,
    pub promotions: usize,
    pub en_passants: usize,
    pub captures: usize,
    pub castles: usize,
    pub checkmates: usize,
}

pub fn perft_nostats(
    board: &mut Board,
    depth: u32,
    should_exit: &impl Fn() -> bool,
) -> Option<u64> {
    if should_exit() {
        return None;
    }

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_all_moves(board);

    if depth == 0 {
        return Some(1);
    }
    if depth == 1 {
        return Some(moves.len() as u64);
    }

    let mut sum = 0;
    for m in moves {
        let undo = board.make_move(m);
        sum += perft_nostats(board, depth - 1, should_exit)?;
        board.undo_move(undo);
    }

    Some(sum)
}

pub fn perft(
    board: &mut Board,
    depth: u32,
    should_exit: &impl Fn() -> bool,
) -> Option<PerftStatistics> {
    if should_exit() {
        return None;
    }

    let movegen = MoveGenerator::with_attacks(board);
    let moves = movegen.generate_all_moves(board);

    let is_checkmate = moves.is_empty() && movegen.is_check();

    if depth == 0 {
        return Some(PerftStatistics {
            divide: Vec::new(),
            node_count: 1,
            checks: if movegen.is_check() { 1 } else { 0 },
            double_checks: if movegen.is_double_check() { 1 } else { 0 },
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: if is_checkmate { 1 } else { 0 },
        });
    }

    Some(
        moves
            .into_iter()
            .map(|m| {
                let undo = board.make_move(m);
                let mut res = perft(board, depth - 1, should_exit)?;
                board.undo_move(undo);

                res.divide = vec![PerftMove {
                    algebraic_move: m.to_algebraic(),
                    nodes: res.node_count,
                }];
                if depth == 1 {
                    match m {
                        Move::Normal {
                            from_square: _,
                            to_square: _,
                            captured_piece,
                            moved_piece: _,
                        } => {
                            if captured_piece.is_some() {
                                res.captures = 1;
                            }
                        }
                        Move::DoublePawn {
                            from_square: _,
                            to_square: _,
                        } => {}
                        Move::EnPassant {
                            from_square: _,
                            to_square: _,
                        } => {
                            res.en_passants = 1;
                            res.captures = 1;
                        }
                        Move::Castle {
                            from_square: _,
                            to_square: _,
                            rook_from_square: _,
                            rook_to_square: _,
                        } => {
                            res.castles += 1;
                        }
                        Move::Promotion {
                            from_square: _,
                            to_square: _,
                            promotion_piece: _,
                            captured_piece,
                        } => {
                            if captured_piece.is_some() {
                                res.captures = 1;
                            }
                            res.promotions = 1;
                        }
                    }
                }
                Some(res)
            })
            .collect::<Option<Vec<_>>>()?
            .into_iter()
            .reduce(|mut a, b| {
                a.divide.extend(b.divide);
                PerftStatistics {
                    divide: a.divide,
                    node_count: a.node_count + b.node_count,
                    checks: a.checks + b.checks,
                    double_checks: a.double_checks + b.double_checks,
                    promotions: a.promotions + b.promotions,
                    en_passants: a.en_passants + b.en_passants,
                    captures: a.captures + b.captures,
                    castles: a.castles + b.castles,
                    checkmates: a.checkmates + b.checkmates,
                }
            })
            .unwrap_or_default(),
    )
}

#[cfg(test)]
mod test {
    use crate::{
        board::Board,
        perft::{PerftStatistics, perft},
    };

    const STARTPOS_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 20,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 400,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 8902,
            checks: 12,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 34,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 197281,
            checks: 469,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 1576,
            castles: 0,
            checkmates: 8,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 4865609,
            checks: 27351,
            double_checks: 0,
            promotions: 0,
            en_passants: 258,
            captures: 82719,
            castles: 0,
            checkmates: 347,
        },
    ];

    #[test]
    fn startpos_depth_1() {
        let depth = 1;
        let mut perft_results = perft(&mut Board::starting_position(), depth, &|| false).unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, STARTPOS_RESULTS[depth as usize])
    }
    #[test]
    fn startpos_depth_2() {
        let depth = 2;
        let mut perft_results = perft(&mut Board::starting_position(), depth, &|| false).unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, STARTPOS_RESULTS[depth as usize])
    }
    #[test]
    fn startpos_depth_3() {
        let depth = 3;
        let mut perft_results = perft(&mut Board::starting_position(), depth, &|| false).unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, STARTPOS_RESULTS[depth as usize])
    }
    #[test]
    fn startpos_depth_4() {
        let depth = 4;
        let mut perft_results = perft(&mut Board::starting_position(), depth, &|| false).unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, STARTPOS_RESULTS[depth as usize])
    }
    #[test]
    fn startpos_depth_5() {
        let depth = 5;
        let mut perft_results = perft(&mut Board::starting_position(), depth, &|| false).unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, STARTPOS_RESULTS[depth as usize])
    }

    const KIWIPETE_FEN: &str =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    const KIWIPETE_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 48,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 8,
            castles: 2,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 2039,
            checks: 3,
            double_checks: 0,
            promotions: 0,
            en_passants: 1,
            captures: 351,
            castles: 91,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 97862,
            checks: 993,
            double_checks: 0,
            promotions: 0,
            en_passants: 45,
            captures: 17102,
            castles: 3162,
            checkmates: 1,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 4085603,
            checks: 25523,
            double_checks: 6,
            promotions: 15172,
            en_passants: 1929,
            captures: 757163,
            castles: 128013,
            checkmates: 43,
        },
    ];

    #[test]
    fn kiwipete_depth_1() {
        let depth = 1;
        let mut perft_results = perft(&mut Board::from_fen(KIWIPETE_FEN).unwrap(), depth, &|| {
            false
        })
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, KIWIPETE_RESULTS[depth as usize])
    }

    #[test]
    fn kiwipete_depth_2() {
        let depth = 2;
        let mut perft_results = perft(&mut Board::from_fen(KIWIPETE_FEN).unwrap(), depth, &|| {
            false
        })
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, KIWIPETE_RESULTS[depth as usize])
    }

    #[test]
    fn kiwipete_depth_3() {
        let depth = 3;
        let mut perft_results = perft(&mut Board::from_fen(KIWIPETE_FEN).unwrap(), depth, &|| {
            false
        })
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, KIWIPETE_RESULTS[depth as usize])
    }

    #[test]
    fn kiwipete_depth_4() {
        let depth = 4;
        let mut perft_results = perft(&mut Board::from_fen(KIWIPETE_FEN).unwrap(), depth, &|| {
            false
        })
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, KIWIPETE_RESULTS[depth as usize])
    }

    const POSITION_3_FEN: &str = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
    const POSITION_3_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 14,
            checks: 2,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 1,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 191,
            checks: 10,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 14,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 2812,
            checks: 267,
            double_checks: 0,
            promotions: 0,
            en_passants: 2,
            captures: 209,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 43238,
            checks: 1680,
            double_checks: 0,
            promotions: 0,
            en_passants: 123,
            captures: 3348,
            castles: 0,
            checkmates: 17,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 674624,
            checks: 52950,
            double_checks: 3,
            promotions: 0,
            en_passants: 1165,
            captures: 52051,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 11030083,
            checks: 452473,
            double_checks: 0,
            promotions: 7552,
            en_passants: 33325,
            captures: 940350,
            castles: 0,
            checkmates: 2733,
        },
    ];

    #[test]
    fn position_3_depth_1() {
        let depth = 1;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    #[test]
    fn position_3_depth_2() {
        let depth = 2;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    #[test]
    fn position_3_depth_3() {
        let depth = 3;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    #[test]
    fn position_3_depth_4() {
        let depth = 4;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    #[test]
    fn position_3_depth_5() {
        let depth = 5;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    #[test]
    fn position_3_depth_6() {
        let depth = 6;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_3_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        assert_eq!(perft_results, POSITION_3_RESULTS[depth as usize])
    }

    const POSITION_4_FEN: &str = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    const POSITION_4_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 6,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 264,
            checks: 10,
            double_checks: 0,
            promotions: 48,
            en_passants: 0,
            captures: 87,
            castles: 6,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 9467,
            checks: 38,
            double_checks: 0,
            promotions: 120,
            en_passants: 4,
            captures: 1021,
            castles: 0,
            checkmates: 22,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 422333,
            checks: 15492,
            double_checks: 0,
            promotions: 60032,
            en_passants: 0,
            captures: 131393,
            castles: 7795,
            checkmates: 5,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 15833292,
            checks: 200568,
            double_checks: 0,
            promotions: 329464,
            en_passants: 6512,
            captures: 2046173,
            castles: 0,
            checkmates: 50562,
        },
    ];

    #[test]
    fn position_4_depth_1() {
        let depth = 1;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_4_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.double_checks = 0; // no reference values available
        assert_eq!(perft_results, POSITION_4_RESULTS[depth as usize])
    }

    #[test]
    fn position_4_depth_2() {
        let depth = 2;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_4_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.double_checks = 0; // no reference values available
        assert_eq!(perft_results, POSITION_4_RESULTS[depth as usize])
    }

    #[test]
    fn position_4_depth_3() {
        let depth = 3;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_4_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.double_checks = 0; // no reference values available
        assert_eq!(perft_results, POSITION_4_RESULTS[depth as usize])
    }

    #[test]
    fn position_4_depth_4() {
        let depth = 4;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_4_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.double_checks = 0; // no reference values available
        assert_eq!(perft_results, POSITION_4_RESULTS[depth as usize])
    }

    #[test]
    fn position_4_depth_5() {
        let depth = 5;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_4_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.double_checks = 0; // no reference values available
        assert_eq!(perft_results, POSITION_4_RESULTS[depth as usize])
    }

    const POSITION_5_FEN: &str = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    const POSITION_5_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 44,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 1486,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 62379,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 2103487,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
    ];

    #[test]
    fn position_5_depth_1() {
        let depth = 1;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_5_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_5_RESULTS[depth as usize])
    }

    #[test]
    fn position_5_depth_2() {
        let depth = 2;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_5_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_5_RESULTS[depth as usize])
    }

    #[test]
    fn position_5_depth_3() {
        let depth = 3;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_5_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_5_RESULTS[depth as usize])
    }

    #[test]
    fn position_5_depth_4() {
        let depth = 4;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_5_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_5_RESULTS[depth as usize])
    }

    const POSITION_6_FEN: &str =
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
    const POSITION_6_RESULTS: &[PerftStatistics] = &[
        PerftStatistics {
            divide: vec![],
            node_count: 1,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 46,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 2079,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 89890,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
        PerftStatistics {
            divide: vec![],
            node_count: 3894594,
            checks: 0,
            double_checks: 0,
            promotions: 0,
            en_passants: 0,
            captures: 0,
            castles: 0,
            checkmates: 0,
        },
    ];

    #[test]
    fn position_6_depth_1() {
        let depth = 1;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_6_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_6_RESULTS[depth as usize])
    }

    #[test]
    fn position_6_depth_2() {
        let depth = 2;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_6_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_6_RESULTS[depth as usize])
    }

    #[test]
    fn position_6_depth_3() {
        let depth = 3;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_6_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_6_RESULTS[depth as usize])
    }

    #[test]
    fn position_6_depth_4() {
        let depth = 4;
        let mut perft_results = perft(
            &mut Board::from_fen(POSITION_6_FEN).unwrap(),
            depth,
            &|| false,
        )
        .unwrap();
        perft_results.divide = vec![];
        perft_results.checks = 0; // no reference values available
        perft_results.double_checks = 0;
        perft_results.promotions = 0;
        perft_results.en_passants = 0;
        perft_results.captures = 0;
        perft_results.castles = 0;
        perft_results.checkmates = 0;
        assert_eq!(perft_results, POSITION_6_RESULTS[depth as usize])
    }
}
