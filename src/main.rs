use thera::{
    self,
    board::Board,
    move_generator::MoveGenerator,
    piece::{Move, Square},
};

fn main() {
    let mut board = Board::starting_position();
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
}
