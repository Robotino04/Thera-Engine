use std::io::BufWriter;

use crate::{
    bitboard::{Bitboard, bitboard},
    board::Board,
    piece::{Color, Direction, Move, Piece},
};

#[derive(Debug)]
pub struct MoveGenerator<const ALL_MOVES: bool> {
    attacks_from_square: [Bitboard; 64],
    attacked_squares: Bitboard,
}

const MAX_MOVES_IN_POSITION: usize = 218;

impl<const ALL_MOVES: bool> MoveGenerator<ALL_MOVES> {
    pub fn with_attacks(board: &mut Board) -> Self {
        board.make_null_move();

        let mut attacks_from_square = [Bitboard(0); 64];

        Self::generate_king_attacks(board, &mut attacks_from_square);
        Self::generate_bishop_attacks(board, &mut attacks_from_square);
        Self::generate_knight_attacks(board, &mut attacks_from_square);
        Self::generate_rook_attacks(board, &mut attacks_from_square);
        Self::generate_queen_attacks(board, &mut attacks_from_square);
        Self::generate_pawn_attacks(board, &mut attacks_from_square);

        board.undo_null_move();

        Self {
            attacks_from_square,
            attacked_squares: attacks_from_square
                .into_iter()
                .reduce(|a, b| a | b)
                .unwrap_or_default(),
        }
    }

    fn targets_to_moves(origin: Bitboard, targets: Bitboard, board: &Board, moves: &mut Vec<Move>) {
        if ALL_MOVES {
            let mut non_captures = targets & !board.all_piece_bitboard();

            while let Some(target) = non_captures.bitscan() {
                moves.push(Move::Normal {
                    from_square: origin,
                    to_square: target,
                    is_capture: false,
                });
            }
        }

        let mut captures = targets & board.get_color_bitboard(board.color_to_move().opposite());

        while let Some(target) = captures.bitscan() {
            moves.push(Move::Normal {
                from_square: origin,
                to_square: target,
                is_capture: true,
            });
        }
    }

    /// https://www.chessprogramming.org/Kogge-Stone_Algorithm
    fn occluded_fill(mut seeds: Bitboard, blockers: Bitboard, dir: Direction) -> Bitboard {
        // prevent wrapping in the other direction because we
        // essentially prevent it after having shifted. So we
        // flip the margins to the other side
        let mut propagators = (!blockers).prevent_wrapping(dir.opposite());
        seeds |= propagators & seeds.rotate(dir, 1);
        propagators &= propagators.rotate(dir, 1);
        seeds |= propagators & seeds.rotate(dir, 2);
        propagators &= propagators.rotate(dir, 2);
        seeds |= propagators & seeds.rotate(dir, 4);

        seeds
    }

    fn generate_king_targets(king: Bitboard) -> Bitboard {
        let middle_row = king.shift(Direction::West) | king | king.shift(Direction::East);
        let targets =
            middle_row.shift(Direction::North) | middle_row | middle_row.shift(Direction::South);

        targets & !king
    }

    fn generate_king_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let king = board.get_piece_bitboard(Piece::King, board.color_to_move());
        let targets = Self::generate_king_targets(king);
        attacks_from_square[king.first_piece_index().unwrap() as usize] |= targets;
    }

    pub fn generate_king_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        let king = board.get_piece_bitboard(Piece::King, board.color_to_move());
        let targets = Self::generate_king_targets(king) & !self.attacked_squares;

        Self::targets_to_moves(king, targets, board, moves);

        if board.can_castle_kingside(board.color_to_move()) {
            let king_move_mask =
                king | king.rotate(Direction::East, 1) | king.rotate(Direction::East, 2);
            let clear_mask = king.rotate(Direction::East, 1) | king.rotate(Direction::East, 2);

            if (king_move_mask & self.attacked_squares).is_empty()
                && (clear_mask & board.all_piece_bitboard()).is_empty()
            {
                moves.push(Move::Castle {
                    from_square: king,
                    to_square: king.rotate(Direction::East, 2),
                    rook_from_square: king.rotate(Direction::East, 3),
                    rook_to_square: king.rotate(Direction::East, 1),
                });
            }
        }
        if board.can_castle_queenside(board.color_to_move()) {
            let king_move_mask =
                king | king.rotate(Direction::West, 1) | king.rotate(Direction::West, 2);
            let clear_mask = king.rotate(Direction::West, 1)
                | king.rotate(Direction::West, 2)
                | king.rotate(Direction::West, 3);

            if (king_move_mask & self.attacked_squares).is_empty()
                && (clear_mask & board.all_piece_bitboard()).is_empty()
            {
                moves.push(Move::Castle {
                    from_square: king,
                    to_square: king.rotate(Direction::West, 2),
                    rook_from_square: king.rotate(Direction::West, 4),
                    rook_to_square: king.rotate(Direction::West, 1),
                });
            }
        }
    }

    fn sliding_moves<const N: usize>(
        board: &Board,
        piece: Bitboard,
        directions: [Direction; N],
    ) -> Bitboard {
        let mut targets = Bitboard::default();
        for dir in directions {
            let non_attacks = Self::occluded_fill(piece, board.all_piece_bitboard(), dir);
            targets |= non_attacks.shift(dir);
        }
        targets & !board.get_color_bitboard(board.color_to_move())
    }

    fn all_sliding_moves<const N: usize>(
        board: &Board,
        moves: &mut Vec<Move>,
        mut pieces: Bitboard,
        directions: [Direction; N],
    ) {
        while let Some(single_piece) = pieces.bitscan() {
            let targets = Self::sliding_moves(board, single_piece, directions);
            Self::targets_to_moves(single_piece, targets, board, moves);
        }
    }
    fn all_sliding_attacks<const N: usize>(
        board: &Board,
        mut pieces: Bitboard,
        directions: [Direction; N],
        attacks_from_square: &mut [Bitboard; 64],
    ) {
        while let Some(single_piece) = pieces.bitscan() {
            let targets = Self::sliding_moves(board, single_piece, directions);
            attacks_from_square[single_piece.first_piece_index().unwrap() as usize] |= targets;
        }
    }

    pub fn generate_rook_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        Self::all_sliding_moves(
            board,
            moves,
            board.get_piece_bitboard(Piece::Rook, board.color_to_move()),
            [
                Direction::North,
                Direction::East,
                Direction::South,
                Direction::West,
            ],
        );
    }
    fn generate_rook_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        Self::all_sliding_attacks(
            board,
            board.get_piece_bitboard(Piece::Rook, board.color_to_move()),
            [
                Direction::North,
                Direction::East,
                Direction::South,
                Direction::West,
            ],
            attacks_from_square,
        );
    }
    pub fn generate_bishop_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        Self::all_sliding_moves(
            board,
            moves,
            board.get_piece_bitboard(Piece::Bishop, board.color_to_move()),
            [
                Direction::NorthEast,
                Direction::NorthWest,
                Direction::SouthEast,
                Direction::SouthWest,
            ],
        );
    }
    pub fn generate_bishop_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        Self::all_sliding_attacks(
            board,
            board.get_piece_bitboard(Piece::Bishop, board.color_to_move()),
            [
                Direction::NorthEast,
                Direction::NorthWest,
                Direction::SouthEast,
                Direction::SouthWest,
            ],
            attacks_from_square,
        );
    }

    pub fn generate_queen_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        Self::all_sliding_moves(
            board,
            moves,
            board.get_piece_bitboard(Piece::Queen, board.color_to_move()),
            [
                Direction::North,
                Direction::East,
                Direction::South,
                Direction::West,
                Direction::NorthEast,
                Direction::NorthWest,
                Direction::SouthEast,
                Direction::SouthWest,
            ],
        );
    }
    fn generate_queen_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        Self::all_sliding_attacks(
            board,
            board.get_piece_bitboard(Piece::Queen, board.color_to_move()),
            [
                Direction::North,
                Direction::East,
                Direction::South,
                Direction::West,
                Direction::NorthEast,
                Direction::NorthWest,
                Direction::SouthEast,
                Direction::SouthWest,
            ],
            attacks_from_square,
        );
    }

    pub fn generate_knight_targets(knight: Bitboard) -> Bitboard {
        let mut targets = Bitboard(0);
        let top_bottom_initial = knight.shift(Direction::East) | knight.shift(Direction::West);
        let left_right_initial = knight.shift(Direction::East).shift(Direction::East)
            | knight.shift(Direction::West).shift(Direction::West);

        targets |= (top_bottom_initial.shift(Direction::North) | left_right_initial)
            .shift(Direction::North)
            | (top_bottom_initial.shift(Direction::South) | left_right_initial)
                .shift(Direction::South);

        targets
    }

    pub fn generate_knight_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        let mut knights = board.get_piece_bitboard(Piece::Knight, board.color_to_move());
        while let Some(knight) = knights.bitscan() {
            let targets = Self::generate_knight_targets(knight);

            Self::targets_to_moves(knight, targets, board, moves);
        }
    }
    pub fn generate_knight_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let mut knights = board.get_piece_bitboard(Piece::Knight, board.color_to_move());
        while let Some(knight) = knights.bitscan() {
            attacks_from_square[knight.first_piece_index().unwrap() as usize] |=
                Self::generate_knight_targets(knight);
        }
    }
    pub fn generate_pawn_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        let forwards = match board.color_to_move() {
            Color::White => Direction::North,
            Color::Black => Direction::South,
        };

        let forwards_west = match board.color_to_move() {
            Color::White => Direction::NorthWest,
            Color::Black => Direction::SouthWest,
        };
        let forwards_east = match board.color_to_move() {
            Color::White => Direction::NorthEast,
            Color::Black => Direction::SouthEast,
        };

        let shifted_baseline = match board.color_to_move() {
            Color::White => bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b11111111
                0b00000000
                0b00000000
            ),
            Color::Black => bitboard!(
                0b00000000
                0b00000000
                0b11111111
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
            ),
        };

        let promotion_line = match board.color_to_move() {
            Color::White => bitboard!(
                0b11111111
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
            ),
            Color::Black => bitboard!(
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b00000000
                0b11111111
            ),
        };

        let mut pawns = board.get_piece_bitboard(Piece::Pawn, board.color_to_move());
        while let Some(pawn) = pawns.bitscan() {
            let mut targets = Bitboard(0);
            targets |= pawn.shift(forwards) & !board.all_piece_bitboard();
            targets |= (targets & shifted_baseline).shift(forwards) & !board.all_piece_bitboard();

            // lock
            let targets = targets;

            let attacks = (pawn.shift(forwards_west) | pawn.shift(forwards_east))
                & board.get_color_bitboard(board.color_to_move().opposite());

            let mut promotion_targets = targets & promotion_line;
            let mut normal_targets = targets & !promotion_line;

            let mut promotion_attacks = attacks & promotion_line;
            let mut normal_attacks = attacks & !promotion_line;

            if ALL_MOVES {
                while let Some(target) = normal_targets.bitscan() {
                    moves.push(Move::Normal {
                        from_square: pawn,
                        to_square: target,
                        is_capture: false,
                    })
                }

                while let Some(target) = promotion_targets.bitscan() {
                    for promotion_piece in [Piece::Bishop, Piece::Knight, Piece::Rook, Piece::Queen]
                    {
                        moves.push(Move::Promotion {
                            from_square: pawn,
                            to_square: target,
                            promotion_piece,
                            is_capture: false,
                        });
                    }
                }
            }

            while let Some(target) = normal_attacks.bitscan() {
                moves.push(Move::Normal {
                    from_square: pawn,
                    to_square: target,
                    is_capture: true,
                })
            }

            while let Some(target) = promotion_attacks.bitscan() {
                for promotion_piece in [Piece::Bishop, Piece::Knight, Piece::Rook, Piece::Queen] {
                    moves.push(Move::Promotion {
                        from_square: pawn,
                        to_square: target,
                        promotion_piece,
                        is_capture: true,
                    });
                }
            }
        }
    }
    fn generate_pawn_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let forwards_west = match board.color_to_move() {
            Color::White => Direction::NorthWest,
            Color::Black => Direction::SouthWest,
        };
        let forwards_east = match board.color_to_move() {
            Color::White => Direction::NorthEast,
            Color::Black => Direction::SouthEast,
        };

        let mut pawns = board.get_piece_bitboard(Piece::Pawn, board.color_to_move());
        while let Some(pawn) = pawns.bitscan() {
            let attacks = (pawn.shift(forwards_west) | pawn.shift(forwards_east))
                & board.get_color_bitboard(board.color_to_move().opposite());

            attacks_from_square[pawn.first_piece_index().unwrap() as usize] |= attacks;
        }
    }
}
