use std::io::{BufRead, BufReader, Write};
use std::process::{Command, Stdio};

use itertools::Itertools;
use thera::{self, board::Board, move_generator::MoveGenerator, piece::Move};

fn perft(board: &mut Board, depth: u32) -> Vec<PerftMove> {
    fn perft_impl(board: &mut Board, depth: u32) -> usize {
        if depth == 0 {
            return 1;
        }
        let mut moves = Vec::new();
        MoveGenerator::<true>::with_attacks(board).generate_all_moves(board, &mut moves);

        if depth == 1 {
            return moves.len();
        }

        moves
            .into_iter()
            .map(|m| {
                let mut board = *board;
                board.make_move(&m);
                perft_impl(&mut board, depth - 1)
            })
            .sum()
    }

    if depth == 0 {
        return Vec::new();
    }
    let mut moves = Vec::new();
    MoveGenerator::<true>::with_attacks(board).generate_all_moves(board, &mut moves);

    moves
        .into_iter()
        .map(|m| {
            let mut board = *board;
            board.make_move(&m);
            let nodes = perft_impl(&mut board, depth - 1);
            PerftMove {
                algebraic_move: m.to_algebraic(),
                nodes,
            }
        })
        .collect_vec()
}

fn perft_stockfish(fen: &str, depth: u32, preapplied_moves: &[String]) -> Vec<PerftMove> {
    let mut cmd = Command::new("stockfish")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        .unwrap();
    let mut stdin = cmd.stdin.as_ref().unwrap();
    if preapplied_moves.is_empty() {
        writeln!(stdin, "position fen {fen}").unwrap();
        println!("position fen {fen}");
    } else {
        writeln!(
            stdin,
            "position fen {fen} moves {}",
            preapplied_moves.join(" ")
        )
        .unwrap();
        println!("position fen {fen} moves {}", preapplied_moves.join(" "))
    }
    writeln!(stdin, "go perft {depth}").unwrap();
    stdin.flush().unwrap();
    let stdout = BufReader::new(cmd.stdout.as_mut().unwrap());

    let stockfish_out = stdout
        .lines()
        .map(|l| l.unwrap())
        .take_while(|line| !line.starts_with("Nodes searched:"))
        .filter(|l| !(l.starts_with("info ") || l.starts_with("Stockfish") || l.trim().is_empty()))
        .map(|line| {
            let [algebraic_move, count] =
                line.split(":").map(|s| s.trim()).collect_array().unwrap();

            PerftMove {
                algebraic_move: algebraic_move.to_string(),
                nodes: count.parse().unwrap(),
            }
        })
        .collect_vec();

    cmd.wait().unwrap();

    stockfish_out
}

#[derive(Clone, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
struct PerftMove {
    algebraic_move: String,
    nodes: usize,
}

fn main() {
    let fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
    let depth = 4;

    let mut board = Board::from_fen(fen).unwrap();
    println!("{}", board.dump_ansi(None));

    let movegen = MoveGenerator::<true>::with_attacks(&mut board);
    let mut moves = Vec::new();
    movegen.generate_all_moves(&board, &mut moves);

    /*
    board.make_move(
        moves
            .iter()
            .find(|actual_move| actual_move.to_algebraic() == "a5b6")
            .unwrap(),
    );
    */

    let targets = moves
        .iter()
        .map(|m| match m {
            Move::Normal { to_square, .. } => *to_square,
            Move::DoublePawn { to_square, .. } => *to_square,
            Move::EnPassant { to_square, .. } => *to_square,
            Move::Promotion { to_square, .. } => *to_square,
            Move::Castle { to_square, .. } => *to_square,
        })
        .reduce(|a, b| a | b)
        .unwrap_or_default();
    println!("{}", board.dump_ansi(Some(targets)));

    let mut move_stack = vec![];

    'next_depth: for depth in (1..=depth).rev() {
        let movegen = MoveGenerator::<true>::with_attacks(&mut board);
        let mut moves = Vec::new();
        movegen.generate_all_moves(&board, &mut moves);

        let mut thera_out = perft(&mut board, depth);
        let mut stockfish_out = perft_stockfish(fen, depth, &move_stack);
        thera_out.sort();
        stockfish_out.sort();

        for m in thera_out {
            if let Some(equiv) = stockfish_out
                .iter()
                .find(|m2| m.algebraic_move == m2.algebraic_move)
                .cloned()
            {
                stockfish_out.retain(|m2| m.algebraic_move != m2.algebraic_move);
                if m.nodes != equiv.nodes {
                    println!("{}", board.dump_ansi(None));
                    println!("{}", board.to_fen());
                    println!(
                        "Move {} has {} nodes for thera, but {} for stockfish",
                        m.algebraic_move, m.nodes, equiv.nodes
                    );
                    println!("Recusing into it");
                    let m = moves
                        .iter()
                        .find(|actual_move| actual_move.to_algebraic() == m.algebraic_move)
                        .expect("thera produced a move it doesn't know about");

                    move_stack.push(m.to_algebraic());
                    board.make_move(m);
                    continue 'next_depth;
                }
            } else {
                println!("{}", board.dump_ansi(None));
                println!("{}", board.to_fen());
                println!("Move {} is missing from stockfish", m.algebraic_move);
                return;
            }
        }
        if let Some(m) = stockfish_out.first() {
            println!("{}", board.dump_ansi(None));
            println!("{}", board.to_fen());
            println!("Move {} is missing from thera", m.algebraic_move);
            return;
        }

        println!("All moves are identical");
    }
}
