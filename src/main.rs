use itertools::Itertools;
use thera::{self, board::Board, move_generator::MoveGenerator, piece::Move};

fn perft(board: &mut Board, depth: u32, is_initial: bool) -> usize {
    if depth == 0 {
        return 1;
    }
    let mut moves = Vec::new();
    MoveGenerator::<true>::with_attacks(board).generate_all_moves(board, &mut moves);

    if depth == 1 && !is_initial {
        return moves.len();
    }

    moves
        .into_iter()
        .map(|m| {
            let mut board = *board;
            board.make_move(&m);
            let c = perft(&mut board, depth - 1, false);
            if is_initial {
                println!("{}: {c}", m.to_algebraic());
            }
            c
        })
        .sum()
}

fn main() {
    let mut board =
        Board::from_fen("rnbqkbnr/1ppppppp/8/p7/Q7/2P5/PP1PPPPP/RNB1KBNR b KQkq - 1 2").unwrap();
    println!("{}", board.dump_ansi(None));

    let movegen = MoveGenerator::<true>::with_attacks(&mut board);
    let mut moves = Vec::new();
    movegen.generate_bishop_moves(&board, &mut moves);

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

    println!("{}", perft(&mut board, 1, true));
}
