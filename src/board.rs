use crate::{
    bitboard::Bitboard,
    piece::{Color, Piece, Square},
};

#[derive(Copy, Clone, Debug)]
pub struct Board {
    pieces: [Bitboard; 6],
    colors: [Bitboard; 2],
}

impl Board {
    pub fn dump_pretty(&self) -> String {
        let mut out = String::new();
        out.push_str("  a b c d e f g h   \n");
        // ().iter().zip(['♟', '♝', '♞', '♜', '♛', '♚'])
        for y in 0..8 {
            for x in 0..8 {}
        }
        out.push_str("  a b c d e f g h   \n");

        out
    }

    pub fn get_piece_at_index(&self, index: Square) -> Option<Piece> {
        Piece::ALL
            .into_iter()
            .find(|&piece| self.pieces[piece as usize].at(index))
    }
    pub fn get_color_at_index(&self, index: Square) -> Option<Color> {
        Color::ALL
            .into_iter()
            .find(|&color| self.colors[color as usize].at(index))
    }
}
