use itertools::Itertools;

use crate::{
    bitboard::{Bitboard, bitboard},
    board::Board,
    piece::{Color, Direction, Move, Piece, Square},
};

#[derive(Debug)]
pub struct MoveGenerator<const ALL_MOVES: bool> {
    attacks_from_square: [Bitboard; 64],
    attacks_to_square: [Bitboard; 64],
    /// NOTE: this does not consider the king as a blocker for sliding pieces.
    /// That's a good thing for king move generation, but maybe not expected for other uses
    attacked_squares: Bitboard,
    allowed_targets: Bitboard,
    pinned: [Bitboard; 8],
    unpinned: Bitboard,
    free_to_move: [Bitboard; 8],
    is_double_check: bool,
    is_check: bool,
}

const MAX_MOVES_IN_POSITION: usize = 218;

impl<const ALL_MOVES: bool> MoveGenerator<ALL_MOVES> {
    /// https://www.chessprogramming.org/Square_Attacked_By#Pure_Calculation
    pub const SQUARES_IN_BETWEEN: [[Bitboard; 64]; 64] = const {
        let mut tmp = [[Bitboard(0); 64]; 64];
        let mut sq1 = 0;
        while sq1 < 64 {
            let mut sq2 = 0;
            while sq2 < 64 {
                let m1: u64 = 0xFFFFFFFFFFFFFFFF;
                let a2a7: u64 = 0x0001010101010100;
                let b2g7: u64 = 0x0040201008040200;
                let h1b7: u64 = 0x0002040810204080;

                let btwn: u64 = (m1 << sq1 as u64) ^ (m1 << sq2 as u64);
                let file: u64 = (sq2 as u64 & 7).wrapping_sub(sq1 as u64 & 7);
                let rank: u64 = ((sq2 as u64 | 7).wrapping_sub(sq1 as u64)) >> 3;
                let mut line: u64 = ((file & 7).wrapping_sub(1)) & a2a7; /* a2a7 if same file */
                line += 2 * (((rank & 7).wrapping_sub(1)) >> 58); /* b1g1 if same rank */
                line += (((rank.wrapping_sub(file)) & 15).wrapping_sub(1)) & b2g7; /* b2g7 if same diagonal */
                line += (((rank.wrapping_add(file)) & 15).wrapping_sub(1)) & h1b7; /* h1b7 if same antidiag */
                line = line.wrapping_mul(btwn & btwn.wrapping_neg()); /* mul acts like shift by smaller square */

                tmp[sq1 as usize][sq2 as usize] = Bitboard(line & btwn); /* return the bits on that line in-between */

                sq2 += 1;
            }
            sq1 += 1;
        }

        tmp
    };

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

        let mut attacks_to_square = [Bitboard(0); 64];
        for (i, mut bb) in attacks_from_square.into_iter().enumerate() {
            while let Some(target_index) = bb.bitscan_index() {
                attacks_to_square[target_index as usize] |=
                    Bitboard::from_square(Square::new(i as u8).unwrap());
            }
        }

        let king = board.get_piece_bitboard(Piece::King, board.color_to_move());
        let king_attackers = attacks_to_square[king.first_piece_index().unwrap() as usize];

        let (is_double_check, is_check, allowed_targets) = if king_attackers.is_empty() {
            (false, false, Bitboard(!0))
        } else if king_attackers.count_ones() >= 2 {
            // double check; no legal other than moving the king
            (true, true, Bitboard(0))
        } else if let attacking_knights =
            king_attackers & board.get_piece_bitboard_colorless(Piece::Knight)
            && !attacking_knights.is_empty()
        {
            // a knight is giving check. Can only capture it
            (false, true, attacking_knights)
        } else {
            // check from a single sliding piece. can only move in between.
            (
                false,
                true,
                Self::SQUARES_IN_BETWEEN[king.first_piece_index().unwrap() as usize]
                    [king_attackers.first_piece_index().unwrap() as usize]
                    | king_attackers,
            )
        };

        let blockers = board.all_piece_bitboard();
        let king = board.get_piece_bitboard(Piece::King, board.color_to_move());

        let mut pinned = Direction::ALL
            .iter()
            .map(|dir| Self::pinned_pieces(board, blockers, king, *dir))
            .collect_array()
            .unwrap();

        // pins are symmetric
        for dir in Direction::ALL {
            pinned[dir.index()] |= pinned[dir.opposite().index()];
        }

        let mut free_to_move = [Bitboard(0); 8];
        for dir in Direction::ALL {
            for dir2 in Direction::ALL {
                if dir == dir2 || dir == dir2.opposite() {
                    continue;
                }
                // If  a square is pinned in dir2, then it can't move in dir.
                // So free_to_move is first the map of non-free_to_move pieces

                free_to_move[dir.index()] |= pinned[dir2.index()];
            }
            free_to_move[dir.index()] = !free_to_move[dir.index()];
        }

        let unpinned = free_to_move.iter().cloned().reduce(|a, b| a & b).unwrap();

        let attacked_squares = attacks_from_square
            .into_iter()
            .reduce(|a, b| a | b)
            .unwrap_or_default();

        Self {
            attacks_from_square,
            attacked_squares,
            attacks_to_square,
            allowed_targets,
            pinned,
            unpinned,
            free_to_move,
            is_double_check,
            is_check,
        }
    }

    pub fn generate_all_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        self.generate_pawn_moves(board, moves);
        self.generate_bishop_moves(board, moves);
        self.generate_knight_moves(board, moves);
        self.generate_rook_moves(board, moves);
        self.generate_queen_moves(board, moves);
        self.generate_king_moves(board, moves);
    }

    pub fn is_check(&self) -> bool {
        self.is_check
    }
    pub fn is_double_check(&self) -> bool {
        self.is_double_check
    }

    fn pinned_pieces(
        board: &Board,
        mut blockers: Bitboard,
        piece: Bitboard,
        dir: Direction,
    ) -> Bitboard {
        let targets = Self::sliding_moves(piece, &[Bitboard(!0); 8], [dir], blockers);

        let attacks = targets & blockers;
        blockers ^= attacks; // remove one line of blockers

        let xrayed_squares = Self::sliding_moves(piece, &[Bitboard(!0); 8], [dir], blockers);

        let endpoints = xrayed_squares & blockers;

        let sliders_in_dir = match dir {
            Direction::North | Direction::East | Direction::South | Direction::West => {
                board.get_piece_bitboard(Piece::Queen, board.color_to_move().opposite())
                    | board.get_piece_bitboard(Piece::Rook, board.color_to_move().opposite())
            }
            Direction::NorthEast
            | Direction::NorthWest
            | Direction::SouthEast
            | Direction::SouthWest => {
                board.get_piece_bitboard(Piece::Queen, board.color_to_move().opposite())
                    | board.get_piece_bitboard(Piece::Bishop, board.color_to_move().opposite())
            }
        };

        // there should only ever be one endpoint in a single direction
        if (endpoints & sliders_in_dir).is_empty() {
            // no one to pin, so nothing is pinned
            Bitboard(0)
        } else {
            // there's someone there
            xrayed_squares
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

        if ALL_MOVES {
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
    }

    fn sliding_moves<const N: usize>(
        piece: Bitboard,
        free_to_move: &[Bitboard; 8],
        directions: [Direction; N],
        blockers: Bitboard,
    ) -> Bitboard {
        let mut targets = Bitboard::default();
        for dir in directions {
            if (free_to_move[dir.index()] & piece).is_empty() {
                continue;
            }
            let non_attacks = Self::occluded_fill(piece, blockers, dir);
            targets |= non_attacks.shift(dir);
        }
        targets
    }

    fn all_sliding_moves<const N: usize>(
        &self,
        board: &Board,
        moves: &mut Vec<Move>,
        mut pieces: Bitboard,
        directions: [Direction; N],
    ) {
        while let Some(single_piece) = pieces.bitscan() {
            let targets = Self::sliding_moves(
                single_piece,
                &self.free_to_move,
                directions,
                board.all_piece_bitboard(),
            ) & self.allowed_targets
                & !board.get_color_bitboard(board.color_to_move());

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
            let targets = Self::sliding_moves(
                single_piece,
                &[Bitboard(!0); 8],
                directions,
                board.all_piece_bitboard()
                    & !board.get_piece_bitboard(Piece::King, board.color_to_move().opposite()),
            );

            attacks_from_square[single_piece.first_piece_index().unwrap() as usize] |= targets;
        }
    }

    pub fn generate_rook_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        moves.reserve_exact(MAX_MOVES_IN_POSITION - moves.len());

        self.all_sliding_moves(
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

        self.all_sliding_moves(
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

        self.all_sliding_moves(
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

        let mut knights =
            board.get_piece_bitboard(Piece::Knight, board.color_to_move()) & self.unpinned;
        while let Some(knight) = knights.bitscan() {
            let targets = Self::generate_knight_targets(knight) & self.allowed_targets;

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
            let targets = (pawn & self.free_to_move[forwards.index()]).shift(forwards)
                & !board.all_piece_bitboard();
            let mut double_targets = (targets & shifted_baseline).shift(forwards)
                & !board.all_piece_bitboard()
                & self.allowed_targets;

            let attack_targets = (pawn & self.free_to_move[forwards_west.index()])
                .shift(forwards_west)
                | (pawn & self.free_to_move[forwards_east.index()]).shift(forwards_east);

            let attacks = attack_targets
                & board.get_color_bitboard(board.color_to_move().opposite())
                & self.allowed_targets;

            let mut promotion_targets = targets & promotion_line & self.allowed_targets;
            let mut normal_targets = targets & !promotion_line & self.allowed_targets;

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
                while let Some(target) = double_targets.bitscan() {
                    moves.push(Move::DoublePawn {
                        from_square: pawn,
                        to_square: target,
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

            // en passant
            if let Some(target) = (attack_targets & board.enpassant_square()).bitscan()
                && !(board.enpassant_square().shift(forwards.opposite()) & self.allowed_targets)
                    .is_empty()
            {
                let king = board.get_piece_bitboard(Piece::King, board.color_to_move());

                let king_square = king.first_piece_square().unwrap();
                let pawn_square = pawn.first_piece_square().unwrap();

                if king_square.row() == pawn_square.row() {
                    // maybe we have both pawns pinned to the king

                    let captured_pawn = target.shift(forwards.opposite());
                    let both_pawns = captured_pawn | pawn;

                    let sliders = board
                        .get_piece_bitboard(Piece::Rook, board.color_to_move().opposite())
                        | board.get_piece_bitboard(Piece::Queen, board.color_to_move().opposite());

                    let blockers = board.all_piece_bitboard() ^ both_pawns;

                    let pin_line = if king_square.column() < pawn_square.column() {
                        Self::occluded_fill(king, blockers, Direction::West).shift(Direction::West)
                    } else {
                        Self::occluded_fill(king, blockers, Direction::East).shift(Direction::East)
                    };

                    if (pin_line & sliders).is_empty() {
                        // we haven't hit a rook or queen after the double-xray
                        moves.push(Move::EnPassant {
                            from_square: pawn,
                            to_square: target,
                        })
                    }
                } else {
                    moves.push(Move::EnPassant {
                        from_square: pawn,
                        to_square: target,
                    })
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
            let attacks = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            attacks_from_square[pawn.first_piece_index().unwrap() as usize] |= attacks;
        }
    }
}
