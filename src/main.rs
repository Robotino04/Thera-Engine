use thera::{self, board::Board, move_generator::MoveGenerator};

fn main() {
    let mut board = Board::from_fen("1n5k/2P2q2/6P1/8/1B2K3/5P1R/3Q2PP/1B2P3 w - - 0 1").unwrap();
    println!("{}", board.dump_ansi(None));

    let movegen = MoveGenerator::new();
    let mut moves = Vec::new();
    movegen.generate_pawn_moves(&board, &mut moves);

    let targets = moves
        .iter()
        .map(|m| m.to_square)
        .reduce(|a, b| a | b)
        .unwrap_or_default();
    println!("{}", board.dump_ansi(Some(targets)));
    println!("{moves:#?}");
    board.make_move(moves.last().unwrap());
    println!("{}", board.dump_ansi(None));
}
