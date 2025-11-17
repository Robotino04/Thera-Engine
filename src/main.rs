use thera::{self, board::Board, move_generator::MoveGenerator, piece::Move};

fn main() {
    let mut board =
        Board::from_fen("r3k2r/pppp2pp/1nbQ1bn1/1p3p2/8/8/PPPPPPPP/RNB1KBNR b KQkq - 0 1").unwrap();
    println!("{}", board.dump_ansi(None));

    let movegen = MoveGenerator::<true>::with_attacks(&mut board);
    let mut moves = Vec::new();
    movegen.generate_king_moves(&board, &mut moves);

    let targets = moves
        .iter()
        .map(|m| match m {
            Move::Normal { to_square, .. } => *to_square,
            Move::Promotion { to_square, .. } => *to_square,
            Move::Castle { to_square, .. } => *to_square,
        })
        .reduce(|a, b| a | b)
        .unwrap_or_default();
    println!("{}", board.dump_ansi(Some(targets)));
}
