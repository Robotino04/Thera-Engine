use thera::{
    self,
    bitboard::Bitboard,
    board::Board,
    piece::{Move, Square},
};

fn main() {
    let mut board = Board::starting_position();
    println!("{}", board.dump_ansi());

    board.make_move(Move::from_algebraic("e2f7").unwrap());
    println!("{}", board.dump_ansi());

    board.make_move(Move::from_algebraic("f7e4").unwrap());
    println!("{}", board.dump_ansi());
}
