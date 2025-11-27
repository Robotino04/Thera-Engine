use itertools::Itertools;

use crate::{
    ansi::Ansi,
    bitboard::{Bitboard, bitboard},
    piece::{Color, Direction, Move, Piece, Square},
};

#[derive(Copy, Clone, Debug)]
pub struct Board {
    pieces: [Bitboard; 6],
    colors: [Bitboard; 2],
    color_to_move: Color,
    can_castle: Bitboard,
    enpassant_square: Bitboard,
    halfmove_clock: u32,
    fullmove_counter: u32,
}

#[derive(Debug)]
pub struct MoveUndoState {
    move_: Move,
    can_castle: Bitboard,
    enpassant_square: Bitboard,
    halfmove_clock: u32,
    fullmove_counter: u32,
}

impl MoveUndoState {
    pub fn move_(&self) -> Move {
        self.move_
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum FenParseError {
    WrongPartCount,
    WrongRowCount,
    InvalidPiece(char),
    TooManyPiecesInRow,
    InvalidTurnColor(char),
    InvalidCastlingRight(char),
    InvalidEnpassantSquare,
    InvalidHalfMoveClock,
    InvalidFullMoveCounter,
}

impl Board {
    pub fn from_fen(fen: &str) -> Result<Self, FenParseError> {
        let mut pieces = [Bitboard(0); 6];
        let mut colors = [Bitboard(0); 2];

        let [
            pieces_str,
            turn_color_str,
            castling_str,
            enpassant_str,
            halfmove_str,
            fullmove_str,
        ] = fen
            .split_whitespace()
            .collect_array()
            .ok_or(FenParseError::WrongPartCount)?;

        let rows: [_; 8] = pieces_str
            .split('/')
            .collect_array()
            .ok_or(FenParseError::WrongRowCount)?;
        for (y, row) in rows.iter().rev().enumerate() {
            let mut x = 0;
            for ch in row.chars() {
                let board = match ch {
                    'p' | 'P' => Some(&mut pieces[Piece::Pawn as usize]),
                    'b' | 'B' => Some(&mut pieces[Piece::Bishop as usize]),
                    'n' | 'N' => Some(&mut pieces[Piece::Knight as usize]),
                    'r' | 'R' => Some(&mut pieces[Piece::Rook as usize]),
                    'q' | 'Q' => Some(&mut pieces[Piece::Queen as usize]),
                    'k' | 'K' => Some(&mut pieces[Piece::King as usize]),

                    '0'..='8' => {
                        x += ch.to_digit(10).unwrap();
                        None
                    }
                    _ => return Err(FenParseError::InvalidPiece(ch)),
                };

                if let Some(board) = board {
                    let square =
                        Square::from_xy(x, y as u32).ok_or(FenParseError::TooManyPiecesInRow)?;
                    board.set(square);
                    if ch.is_ascii_lowercase() {
                        colors[Color::Black as usize].set(square);
                    } else {
                        colors[Color::White as usize].set(square);
                    }
                    x += 1;
                }
            }
        }

        let color_to_play = match turn_color_str {
            "w" => Color::White,
            "b" => Color::Black,
            c => return Err(FenParseError::InvalidTurnColor(c.chars().next().unwrap())),
        };

        let mut can_castle = Bitboard(0);
        if castling_str == "-" {
        } else {
            for ch in castling_str.chars() {
                match ch {
                    'q' => {
                        can_castle |= bitboard!(
                            0b10001000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                        )
                    }
                    'Q' => {
                        can_castle |= bitboard!(
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b10001000
                        )
                    }
                    'k' => {
                        can_castle |= bitboard!(
                            0b00001001
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                        )
                    }
                    'K' => {
                        can_castle |= bitboard!(
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00000000
                            0b00001001
                        )
                    }
                    _ => return Err(FenParseError::InvalidCastlingRight(ch)),
                }
            }
        }

        let enpassant_square = if enpassant_str == "-" {
            Bitboard(0)
        } else {
            Bitboard::from_square(
                Square::from_algebraic(enpassant_str)
                    .ok_or(FenParseError::InvalidEnpassantSquare)?,
            )
        };

        let halfmove_clock = halfmove_str
            .parse()
            .map_err(|_| FenParseError::InvalidHalfMoveClock)?;
        let fullmove_counter = fullmove_str
            .parse()
            .map_err(|_| FenParseError::InvalidFullMoveCounter)?;

        Ok(Board {
            pieces,
            colors,
            color_to_move: color_to_play,
            can_castle,
            enpassant_square,
            halfmove_clock,
            fullmove_counter,
        })
    }

    pub fn starting_position() -> Self {
        Self::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
            .expect("The starting FEN should always be valid")
    }

    pub fn dump(&self, white_background: bool) -> String {
        let mut out = String::new();
        out.push_str("   a b c d e f g h\n");
        for y in (0..8).rev() {
            let mut inside_line = String::new();
            for x in 0..8 {
                let square = Square::from_xy(x, y).unwrap();

                let piece = match self.get_piece_at_index(square).and_then(|piece| {
                    self.get_color_at_index(square).map(|color| {
                        (
                            if white_background {
                                color
                            } else {
                                color.opposite()
                            },
                            piece,
                        )
                    })
                }) {
                    None => ' ',
                    Some((Color::Black, Piece::Pawn)) => '♟',
                    Some((Color::Black, Piece::Bishop)) => '♝',
                    Some((Color::Black, Piece::Knight)) => '♞',
                    Some((Color::Black, Piece::Rook)) => '♜',
                    Some((Color::Black, Piece::Queen)) => '♛',
                    Some((Color::Black, Piece::King)) => '♚',

                    Some((Color::White, Piece::Pawn)) => '♙',
                    Some((Color::White, Piece::Bishop)) => '♗',
                    Some((Color::White, Piece::Knight)) => '♘',
                    Some((Color::White, Piece::Rook)) => '♖',
                    Some((Color::White, Piece::Queen)) => '♕',
                    Some((Color::White, Piece::King)) => '♔',
                };

                inside_line.push_str(&format!("{piece} "));
            }
            out.push_str(&format!(" {ly} {inside_line}{ly}\n", ly = y + 1));
        }
        out.push_str("   a b c d e f g h\n");

        out
    }
    #[must_use = "This function only generates the string representation. It must still be printed."]
    pub fn dump_ansi(&self, highlights: Option<Bitboard>) -> String {
        let dark_color = (184, 135, 98);
        let light_color = (233, 207, 174);
        let highligh_color = (0, 255, 255);
        let highlight_alpha = 0.4;

        fn overlay_color(
            bg: (i32, i32, i32),
            overlay: (i32, i32, i32),
            alpha: f32,
        ) -> (i32, i32, i32) {
            (
                (bg.0 as f32 * (1.0 - alpha) + overlay.0 as f32 * alpha) as i32,
                (bg.1 as f32 * (1.0 - alpha) + overlay.1 as f32 * alpha) as i32,
                (bg.2 as f32 * (1.0 - alpha) + overlay.2 as f32 * alpha) as i32,
            )
        }

        let mut out = String::new();
        out.push_str("   a b c d e f g h\n");
        for y in (0..8).rev() {
            let mut inside_line = String::new();
            for x in 0..8 {
                let square = Square::from_xy(x, y).unwrap();

                let piece_color = match self.get_color_at_index(square) {
                    None => "",
                    Some(Color::White) => &Ansi::fg_color24(255, 255, 255),
                    Some(Color::Black) => &Ansi::fg_color24(0, 0, 0),
                };
                let (this_square_color, next_square_color) = if (x + y) % 2 == 1 {
                    (dark_color, light_color)
                } else {
                    (light_color, dark_color)
                };

                let this_square_color = if let Some(highlights) = highlights
                    && highlights.at(square)
                {
                    overlay_color(this_square_color, highligh_color, highlight_alpha)
                } else {
                    this_square_color
                };
                let next_square_color = if let Some(highlights) = highlights
                    && let Some(next_square) = Square::new((square as u8).saturating_sub(1))
                    && highlights.at(next_square)
                {
                    overlay_color(next_square_color, highligh_color, highlight_alpha)
                } else {
                    next_square_color
                };

                let (this_square_color_fg, this_square_color_bg, next_square_color_fg) = (
                    Ansi::fg_color24(
                        this_square_color.0 as u8,
                        this_square_color.1 as u8,
                        this_square_color.2 as u8,
                    ),
                    Ansi::bg_color24(
                        this_square_color.0 as u8,
                        this_square_color.1 as u8,
                        this_square_color.2 as u8,
                    ),
                    Ansi::fg_color24(
                        next_square_color.0 as u8,
                        next_square_color.1 as u8,
                        next_square_color.2 as u8,
                    ),
                );

                let piece = match self.get_piece_at_index(square) {
                    None => ' ',
                    Some(Piece::Pawn) => '♟',
                    Some(Piece::Bishop) => '♝',
                    Some(Piece::Knight) => '♞',
                    Some(Piece::Rook) => '♜',
                    Some(Piece::Queen) => '♛',
                    Some(Piece::King) => '♚',
                };

                let reset = Ansi::reset();
                if x == 0 {
                    inside_line.push_str(&format!("{this_square_color_fg}▐{reset}"));
                }
                inside_line.push_str(&format!(
                    "{this_square_color_bg}{piece_color}{piece}{reset}",
                    // ▌
                ));
                if x == 7 {
                    inside_line.push_str(&format!("{this_square_color_fg}▌{reset}"));
                } else {
                    inside_line.push_str(&format!(
                        "{this_square_color_bg}{next_square_color_fg}▐{reset}"
                    ));
                }
            }
            out.push_str(&format!(" {ly}{inside_line}{ly}\n", ly = y + 1));
        }
        out.push_str("   a b c d e f g h\n");

        out
    }

    pub fn get_piece_from_bitboard(&self, bb: Bitboard) -> Option<Piece> {
        Piece::ALL
            .into_iter()
            .find(|&piece| !(self.pieces[piece as usize] & bb).is_empty())
    }
    pub fn get_color_from_bitboard(&self, bb: Bitboard) -> Option<Color> {
        Color::ALL
            .into_iter()
            .find(|&color| !(self.colors[color as usize] & bb).is_empty())
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

    #[must_use]
    pub fn make_move(&mut self, m: Move) -> MoveUndoState {
        let undo_state = MoveUndoState {
            move_: m,
            can_castle: self.can_castle,
            enpassant_square: self.enpassant_square,
            halfmove_clock: self.halfmove_clock,
            fullmove_counter: self.fullmove_counter,
        };

        match m {
            Move::Normal {
                from_square,
                to_square,
                captured_piece,
                moved_piece,
            } => {
                if let Some(captured_piece) = captured_piece {
                    self.colors[self.color_to_move().opposite() as usize] ^= to_square;
                    self.pieces[captured_piece as usize] ^= to_square;
                }

                // toggle piece into place
                self.colors[self.color_to_move() as usize] ^= to_square | from_square;
                self.pieces[moved_piece as usize] ^= to_square | from_square;

                self.can_castle &= !to_square & !from_square;
                self.enpassant_square = Bitboard(0);
            }
            Move::DoublePawn {
                from_square,
                to_square,
            } => {
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;
                self.pieces[Piece::Pawn as usize] ^= from_square | to_square;

                self.enpassant_square = Bitboard::from_square(
                    Square::new(
                        (from_square.first_piece_index().unwrap() as u8
                            + to_square.first_piece_index().unwrap() as u8)
                            / 2,
                    )
                    .unwrap(),
                );
            }
            Move::EnPassant {
                from_square,
                to_square,
            } => {
                let captured_pawn = match self.color_to_move() {
                    Color::White => to_square.wrapping_shift(Direction::South, 1),
                    Color::Black => to_square.wrapping_shift(Direction::North, 1),
                };

                // toggle pawn to new place
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;
                self.pieces[Piece::Pawn as usize] ^= from_square | to_square;

                // remove the opponent pawn
                self.colors[self.color_to_move().opposite() as usize] ^= captured_pawn;
                self.pieces[Piece::Pawn as usize] ^= captured_pawn;

                self.enpassant_square = Bitboard(0);
            }
            Move::Promotion {
                from_square,
                to_square,
                promotion_piece,
                captured_piece,
            } => {
                if let Some(captured_piece) = captured_piece {
                    self.colors[self.color_to_move().opposite() as usize] ^= to_square;
                    self.pieces[captured_piece as usize] ^= to_square;
                }

                self.pieces[Piece::Pawn as usize] ^= from_square;
                self.colors[self.color_to_move() as usize] ^= from_square;

                self.pieces[promotion_piece as usize] ^= to_square;
                self.colors[self.color_to_move() as usize] ^= to_square;

                self.can_castle &= !to_square & !from_square;
                self.enpassant_square = Bitboard(0);
            }
            Move::Castle {
                from_square,
                to_square,
                rook_from_square,
                rook_to_square,
            } => {
                // toggle king into place
                self.pieces[Piece::King as usize] ^= from_square | to_square;
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;

                // toggle rook into place
                self.pieces[Piece::Rook as usize] ^= rook_from_square | rook_to_square;
                self.colors[self.color_to_move() as usize] ^= rook_from_square | rook_to_square;

                self.can_castle &= !to_square & !from_square;
                self.enpassant_square = Bitboard(0);
            }
        }
        self.color_to_move = self.color_to_move.opposite();

        undo_state
    }

    pub fn undo_move(&mut self, undo_data: MoveUndoState) {
        self.color_to_move = self.color_to_move.opposite();

        let MoveUndoState {
            move_,
            can_castle,
            enpassant_square,
            halfmove_clock,
            fullmove_counter,
        } = undo_data;

        self.can_castle = can_castle;
        self.enpassant_square = enpassant_square;
        self.halfmove_clock = halfmove_clock;
        self.fullmove_counter = fullmove_counter;

        // NOTE: xor doesn't care about the order of toggling bits. So this code is mostly kept
        // identical to make_move
        match move_ {
            Move::Normal {
                from_square,
                to_square,
                captured_piece,
                moved_piece,
            } => {
                if let Some(captured_piece) = captured_piece {
                    self.colors[self.color_to_move().opposite() as usize] ^= to_square;
                    self.pieces[captured_piece as usize] ^= to_square;
                }

                // toggle piece into place
                self.colors[self.color_to_move() as usize] ^= to_square | from_square;
                self.pieces[moved_piece as usize] ^= to_square | from_square;
            }
            Move::DoublePawn {
                from_square,
                to_square,
            } => {
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;
                self.pieces[Piece::Pawn as usize] ^= from_square | to_square;
            }
            Move::EnPassant {
                from_square,
                to_square,
            } => {
                let captured_pawn = match self.color_to_move() {
                    Color::White => to_square.wrapping_shift(Direction::South, 1),
                    Color::Black => to_square.wrapping_shift(Direction::North, 1),
                };

                // toggle pawn to new place
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;
                self.pieces[Piece::Pawn as usize] ^= from_square | to_square;

                // remove the opponent pawn
                self.colors[self.color_to_move().opposite() as usize] ^= captured_pawn;
                self.pieces[Piece::Pawn as usize] ^= captured_pawn;
            }
            Move::Promotion {
                from_square,
                to_square,
                promotion_piece,
                captured_piece,
            } => {
                if let Some(captured_piece) = captured_piece {
                    self.colors[self.color_to_move().opposite() as usize] ^= to_square;
                    self.pieces[captured_piece as usize] ^= to_square;
                }

                self.pieces[Piece::Pawn as usize] ^= from_square;
                self.colors[self.color_to_move() as usize] ^= from_square;

                self.pieces[promotion_piece as usize] ^= to_square;
                self.colors[self.color_to_move() as usize] ^= to_square;
            }
            Move::Castle {
                from_square,
                to_square,
                rook_from_square,
                rook_to_square,
            } => {
                // toggle king into place
                self.pieces[Piece::King as usize] ^= from_square | to_square;
                self.colors[self.color_to_move() as usize] ^= from_square | to_square;

                // toggle rook into place
                self.pieces[Piece::Rook as usize] ^= rook_from_square | rook_to_square;
                self.colors[self.color_to_move() as usize] ^= rook_from_square | rook_to_square;
            }
        }
    }

    pub fn pawns(&self, color: Color) -> Bitboard {
        self.pieces[Piece::Pawn as usize] & self.colors[color as usize]
    }
    pub fn bishops(&self, color: Color) -> Bitboard {
        self.pieces[Piece::Bishop as usize] & self.colors[color as usize]
    }
    pub fn knights(&self, color: Color) -> Bitboard {
        self.pieces[Piece::Knight as usize] & self.colors[color as usize]
    }
    pub fn rooks(&self, color: Color) -> Bitboard {
        self.pieces[Piece::Rook as usize] & self.colors[color as usize]
    }
    pub fn queens(&self, color: Color) -> Bitboard {
        self.pieces[Piece::Queen as usize] & self.colors[color as usize]
    }
    pub fn king(&self, color: Color) -> Bitboard {
        self.pieces[Piece::King as usize] & self.colors[color as usize]
    }

    pub fn get_piece_bitboard(&self, piece: Piece, color: Color) -> Bitboard {
        self.pieces[piece as usize] & self.colors[color as usize]
    }
    pub fn get_piece_bitboard_colorless(&self, piece: Piece) -> Bitboard {
        self.pieces[piece as usize]
    }
    pub fn all_piece_bitboard(&self) -> Bitboard {
        self.get_color_bitboard(Color::White) | self.get_color_bitboard(Color::Black)
    }
    pub fn get_color_bitboard(&self, color: Color) -> Bitboard {
        self.colors[color as usize]
    }

    pub fn color_to_move(&self) -> Color {
        self.color_to_move
    }

    pub fn make_null_move(&mut self) {
        self.color_to_move = self.color_to_move.opposite();
    }
    pub fn undo_null_move(&mut self) {
        self.color_to_move = self.color_to_move.opposite();
    }

    pub fn can_castle_kingside(&self, color: Color) -> bool {
        let bb = match color {
            Color::Black => bitboard!(
                0b00001001
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
            ),
            Color::White => bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00001001
            ),
        };

        self.can_castle & bb == bb
    }
    pub fn can_castle_queenside(&self, color: Color) -> bool {
        let bb = match color {
            Color::Black => bitboard!(
                0b10001000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
            ),
            Color::White => bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b10001000
            ),
        };

        self.can_castle & bb == bb
    }
    pub fn enpassant_square(&self) -> Bitboard {
        self.enpassant_square
    }

    pub fn to_fen(&self) -> String {
        let mut out = String::new();
        let mut empty_count = 0;
        for y in (0..8).rev() {
            for x in 0..8 {
                let square = Square::from_xy(x, y).unwrap();
                let piece_char = match self.get_piece_at_index(square) {
                    Some(Piece::Pawn) => "p",
                    Some(Piece::Bishop) => "b",
                    Some(Piece::Knight) => "n",
                    Some(Piece::Rook) => "r",
                    Some(Piece::Queen) => "q",
                    Some(Piece::King) => "k",
                    None => {
                        empty_count += 1;
                        continue;
                    }
                };
                if empty_count > 0 {
                    out.push_str(&empty_count.to_string());
                    empty_count = 0;
                }

                out.push_str(&match self.get_color_at_index(square).unwrap() {
                    Color::White => piece_char.to_uppercase(),
                    Color::Black => piece_char.to_string(),
                });
            }
            if empty_count > 0 {
                out.push_str(&empty_count.to_string());
                empty_count = 0;
            }

            if y != 0 {
                out.push('/');
            }
        }

        out.push(' ');
        match self.color_to_move() {
            Color::White => out.push('w'),
            Color::Black => out.push('b'),
        }

        out.push(' ');
        let mut can_castle_at_all = false;
        if self.can_castle_kingside(Color::White) {
            out.push('K');
            can_castle_at_all = true;
        }
        if self.can_castle_queenside(Color::White) {
            out.push('Q');
            can_castle_at_all = true;
        }
        if self.can_castle_kingside(Color::Black) {
            out.push('k');
            can_castle_at_all = true;
        }
        if self.can_castle_queenside(Color::Black) {
            out.push('q');
            can_castle_at_all = true;
        }

        if !can_castle_at_all {
            out.push('-');
        }

        out.push(' ');
        if let Some(enpassant_square) = self.enpassant_square.first_piece_square() {
            out.push_str(enpassant_square.to_algebraic());
        } else {
            out.push('-');
        }

        out.push(' ');
        out.push_str(&self.halfmove_clock.to_string());
        out.push(' ');
        out.push_str(&self.fullmove_counter.to_string());

        out
    }
}
