use crate::{
    bitboard::{Bitboard, bitboard},
    board::Board,
    piece::{Color, Direction, Move, Piece},
};

#[derive(Debug)]
pub struct MoveGenerator {}

const MAX_MOVES_IN_POSITION: usize = 218;

impl MoveGenerator {
    pub fn new() -> Self {
        Self {}
    }

    fn targets_to_moves(origin: Bitboard, mut targets: Bitboard, moves: &mut Vec<Move>) {
        while let Some(target) = targets.bitscan() {
            moves.push(Move {
                from_square: origin,
                to_square: target,
                promotion_piece: None,
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

    pub fn generate_king_moves(&mut self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        let king = board.get_piece_bitboard(Piece::King, board.color_to_move());
        let middle_row = king.shift(Direction::West) | king | king.shift(Direction::East);
        let targets =
            middle_row.shift(Direction::North) | middle_row | middle_row.shift(Direction::South);
        let targets = targets & !king;
        MoveGenerator::targets_to_moves(
            king,
            targets & !board.get_color_bitboard(board.color_to_move()),
            moves,
        );
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
            Self::targets_to_moves(
                single_piece,
                Self::sliding_moves(board, single_piece, directions),
                moves,
            );
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

    pub fn generate_knight_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        let mut knights = board.get_piece_bitboard(Piece::Knight, board.color_to_move());
        while let Some(knight) = knights.bitscan() {
            let mut targets = Bitboard(0);
            let top_bottom_initial = knight.shift(Direction::East) | knight.shift(Direction::West);
            let left_right_initial = knight.shift(Direction::East).shift(Direction::East)
                | knight.shift(Direction::West).shift(Direction::West);

            targets |= (top_bottom_initial.shift(Direction::North) | left_right_initial)
                .shift(Direction::North)
                | (top_bottom_initial.shift(Direction::South) | left_right_initial)
                    .shift(Direction::South);

            Self::targets_to_moves(
                knight,
                targets & !board.get_color_bitboard(board.color_to_move()),
                moves,
            );
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

            let attacks = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            targets |= attacks & board.get_color_bitboard(board.color_to_move().opposite());

            let mut promotion_targets = targets & promotion_line;
            let normal_targets = targets & !promotion_line;

            Self::targets_to_moves(pawn, normal_targets, moves);

            while let Some(target) = promotion_targets.bitscan() {
                for promotion_piece in [Piece::Bishop, Piece::Knight, Piece::Rook, Piece::Queen] {
                    moves.push(Move {
                        from_square: pawn,
                        to_square: target,
                        promotion_piece: Some(promotion_piece),
                    });
                }
            }
        }
    }
}

impl Default for MoveGenerator {
    fn default() -> Self {
        Self::new()
    }
}
