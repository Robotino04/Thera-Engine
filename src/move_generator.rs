use std::sync::LazyLock;

use itertools::Itertools;

use crate::{
    bitboard::{Bitboard, bitboard},
    board::Board,
    piece::{Color, Direction, Move, Piece},
};

#[derive(Debug)]
pub struct MoveGenerator<const ALL_MOVES: bool> {
    attacks_from_square: [Bitboard; 64],
    attacks_to_square: [Bitboard; 64],
    /// NOTE: this does not consider the king as a blocker for sliding pieces.
    /// That's a good thing for king move generation, but maybe not expected for other uses
    attacked_squares: Bitboard,
    allowed_targets: [Bitboard; 64],
    is_double_check: bool,
    is_check: bool,
}

const MAX_MOVES_IN_POSITION: usize = 218;

#[derive(Clone, Copy, Debug)]
pub struct MagicTableEntry {
    pub magic: u64,
    pub mask: Bitboard,
    pub bits_used: u32,
}

impl MagicTableEntry {
    pub const fn apply(&self, occupancy: Bitboard) -> u64 {
        ((self.mask.const_and(occupancy)).0.wrapping_mul(self.magic)) >> (64 - self.bits_used)
    }
}

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
        let mut attacked_squares = Bitboard(0);
        for (i, mut bb) in attacks_from_square.iter().cloned().enumerate() {
            attacked_squares |= bb;
            while let Some(target_index) = bb.bitscan_index() {
                attacks_to_square[target_index as usize] |= Bitboard(1 << i);
            }
        }

        let king = board.king(board.color_to_move());
        let king_square = king.first_piece_index().unwrap_or_else(|| {
            let current_fen = board.to_fen();
            unreachable!("{current_fen}: position doesn't have a king");
        });
        let king_attackers = attacks_to_square[king_square as usize];

        let (is_double_check, is_check, allowed_targets) = if king_attackers.is_empty() {
            (false, false, Bitboard(!0))
        } else if king_attackers.count_ones() >= 2 {
            // double check; no legal moves other than moving the king
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
                Self::SQUARES_IN_BETWEEN[king_square as usize]
                    [king_attackers.first_piece_index().unwrap() as usize]
                    | king_attackers,
            )
        };

        let blockers = board.all_piece_bitboard();

        let mut target_restrictions = [allowed_targets; 64];
        for dir in Direction::ALL {
            let pinned_squares = Self::pinned_pieces(board, blockers, king, dir);
            let mut pinned_squares2 = pinned_squares;
            while let Some(square) = pinned_squares2.bitscan_index() {
                target_restrictions[square as usize] &= pinned_squares;
            }
        }

        let attacked_squares = attacks_from_square
            .iter()
            .cloned()
            .reduce(|a, b| a | b)
            .unwrap_or_default();

        Self {
            attacks_from_square,
            attacked_squares,
            attacks_to_square,
            allowed_targets: target_restrictions,
            is_double_check,
            is_check,
        }
    }

    pub fn generate_all_moves(&self, board: &Board) -> Vec<Move> {
        if !self.is_double_check {
            let mut moves = Vec::with_capacity(MAX_MOVES_IN_POSITION);

            self.generate_pawn_moves(board, &mut moves);
            self.generate_bishop_moves(board, &mut moves);
            self.generate_knight_moves(board, &mut moves);
            self.generate_rook_moves(board, &mut moves);
            self.generate_queen_moves(board, &mut moves);
            self.generate_king_moves(board, &mut moves);

            moves
        } else {
            let mut moves = Vec::with_capacity(8);

            self.generate_king_moves(board, &mut moves);

            moves
        }
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
        let targets = Self::occluded_fill(piece, blockers, dir).shift(dir);

        let attacks = targets & blockers;
        blockers ^= attacks; // remove one line of blockers

        let xrayed_squares = Self::occluded_fill(piece, blockers, dir).shift(dir);

        let endpoints = xrayed_squares & blockers;

        let sliders_in_dir = match dir {
            Direction::North | Direction::East | Direction::South | Direction::West => {
                board.queens(board.color_to_move().opposite())
                    | board.rooks(board.color_to_move().opposite())
            }
            Direction::NorthEast
            | Direction::NorthWest
            | Direction::SouthEast
            | Direction::SouthWest => {
                board.queens(board.color_to_move().opposite())
                    | board.bishops(board.color_to_move().opposite())
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

    fn targets_to_moves(
        origin: Bitboard,
        moved_piece: Piece,
        targets: Bitboard,
        board: &Board,
        moves: &mut Vec<Move>,
    ) {
        if ALL_MOVES {
            let mut non_captures = targets & !board.all_piece_bitboard();

            while let Some(target) = non_captures.bitscan() {
                moves.push(Move::Normal {
                    from_square: origin,
                    to_square: target,
                    captured_piece: None,
                    moved_piece,
                });
            }
        }

        let mut captures = targets & board.get_color_bitboard(board.color_to_move().opposite());

        while let Some(target) = captures.bitscan() {
            moves.push(Move::Normal {
                from_square: origin,
                to_square: target,
                captured_piece: Some(board.get_piece_from_bitboard(target).unwrap()),
                moved_piece,
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

        targets ^ king
    }

    fn generate_king_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let king = board.king(board.color_to_move());
        let targets = Self::generate_king_targets(king);
        attacks_from_square[king.first_piece_index().unwrap() as usize] |= targets;
    }

    fn generate_king_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        let king = board.king(board.color_to_move());
        let targets = Self::generate_king_targets(king) & !self.attacked_squares;

        Self::targets_to_moves(king, Piece::King, targets, board, moves);

        if ALL_MOVES {
            if board.can_castle_kingside(board.color_to_move()) {
                let king_move_mask = king
                    | king.wrapping_shift(Direction::East, 1)
                    | king.wrapping_shift(Direction::East, 2);
                let clear_mask = king.wrapping_shift(Direction::East, 1)
                    | king.wrapping_shift(Direction::East, 2);

                if (king_move_mask & self.attacked_squares).is_empty()
                    && (clear_mask & board.all_piece_bitboard()).is_empty()
                {
                    moves.push(Move::Castle {
                        from_square: king,
                        to_square: king.wrapping_shift(Direction::East, 2),
                        rook_from_square: king.wrapping_shift(Direction::East, 3),
                        rook_to_square: king.wrapping_shift(Direction::East, 1),
                    });
                }
            }
            if board.can_castle_queenside(board.color_to_move()) {
                let king_move_mask = king
                    | king.wrapping_shift(Direction::West, 1)
                    | king.wrapping_shift(Direction::West, 2);
                let clear_mask = king.wrapping_shift(Direction::West, 1)
                    | king.wrapping_shift(Direction::West, 2)
                    | king.wrapping_shift(Direction::West, 3);

                if (king_move_mask & self.attacked_squares).is_empty()
                    && (clear_mask & board.all_piece_bitboard()).is_empty()
                {
                    moves.push(Move::Castle {
                        from_square: king,
                        to_square: king.wrapping_shift(Direction::West, 2),
                        rook_from_square: king.wrapping_shift(Direction::West, 4),
                        rook_to_square: king.wrapping_shift(Direction::West, 1),
                    });
                }
            }
        }
    }

    fn sliding_moves<const N: usize>(
        piece: Bitboard,
        directions: [Direction; N],
        blockers: Bitboard,
    ) -> Bitboard {
        let mut targets = Bitboard::default();
        for dir in directions {
            let non_attacks = Self::occluded_fill(piece, blockers, dir);
            targets |= non_attacks.shift(dir);
        }
        targets
    }

    fn rook_magic_bitboard(from_index: u32, occupancy: Bitboard) -> Bitboard {
        ROOK_MAGIC_TABLE[from_index as usize]
            [ROOK_MAGIC_ENTRIES[from_index as usize].apply(occupancy) as usize]
    }
    fn bishop_magic_bitboard(from_index: u32, occupancy: Bitboard) -> Bitboard {
        BISHOP_MAGIC_TABLE[from_index as usize]
            [BISHOP_MAGIC_ENTRIES[from_index as usize].apply(occupancy) as usize]
    }

    fn generate_rook_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        let mut pieces = board.rooks(board.color_to_move());

        while let Some(single_piece) = pieces.bitscan_index() {
            let occupancy = board.all_piece_bitboard();

            let targets = Self::rook_magic_bitboard(single_piece, occupancy)
                & self.allowed_targets[single_piece as usize]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves(
                Bitboard(1 << single_piece),
                Piece::Rook,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_rook_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let mut rooks = board.rooks(board.color_to_move());
        while let Some(rook) = rooks.bitscan_index() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            attacks_from_square[rook as usize] |= Self::rook_magic_bitboard(rook, occupancy);
        }
    }
    fn generate_bishop_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        let mut pieces = board.bishops(board.color_to_move());

        while let Some(single_piece) = pieces.bitscan_index() {
            let occupancy = board.all_piece_bitboard();
            let targets = Self::bishop_magic_bitboard(single_piece, occupancy)
                & self.allowed_targets[single_piece as usize]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves(
                Bitboard(1 << single_piece),
                Piece::Bishop,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_bishop_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let mut bishops = board.bishops(board.color_to_move());
        while let Some(bishop) = bishops.bitscan_index() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            let entry = BISHOP_MAGIC_TABLE[bishop as usize]
                [BISHOP_MAGIC_ENTRIES[bishop as usize].apply(occupancy) as usize];

            attacks_from_square[bishop as usize] |= entry;
        }
    }

    fn generate_queen_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        let mut pieces = board.queens(board.color_to_move());

        while let Some(single_piece) = pieces.bitscan_index() {
            let occupancy = board.all_piece_bitboard();
            let targets = (Self::bishop_magic_bitboard(single_piece, occupancy)
                | Self::rook_magic_bitboard(single_piece, occupancy))
                & self.allowed_targets[single_piece as usize]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves(
                Bitboard(1 << single_piece),
                Piece::Queen,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_queen_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let mut queens = board.queens(board.color_to_move());
        while let Some(queen) = queens.bitscan_index() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            let rook_entry = ROOK_MAGIC_TABLE[queen as usize]
                [ROOK_MAGIC_ENTRIES[queen as usize].apply(occupancy) as usize];
            let bishop_entry = BISHOP_MAGIC_TABLE[queen as usize]
                [BISHOP_MAGIC_ENTRIES[queen as usize].apply(occupancy) as usize];

            attacks_from_square[queen as usize] |= rook_entry | bishop_entry;
        }
    }

    fn generate_knight_targets(knight: Bitboard) -> Bitboard {
        const LUT: [Bitboard; 64] = {
            let mut lut = [Bitboard(0); 64];

            let mut square = 0;
            while square < 64 {
                let knight = Bitboard(1 << square);

                let top_bottom_initial = knight
                    .shift(Direction::East)
                    .const_or(knight.shift(Direction::West));
                let left_right_initial = knight
                    .shift(Direction::East)
                    .shift(Direction::East)
                    .const_or(knight.shift(Direction::West).shift(Direction::West));

                let targets = (top_bottom_initial
                    .shift(Direction::North)
                    .const_or(left_right_initial))
                .shift(Direction::North)
                .const_or(
                    (top_bottom_initial
                        .shift(Direction::South)
                        .const_or(left_right_initial))
                    .shift(Direction::South),
                );

                lut[square as usize] = targets;

                square += 1;
            }

            lut
        };

        LUT[knight.first_piece_index().unwrap() as usize]
    }

    fn generate_knight_moves(&self, board: &Board, moves: &mut Vec<Move>) {
        let mut knights = board.knights(board.color_to_move());
        while let Some(knight) = knights.bitscan() {
            let targets = Self::generate_knight_targets(knight)
                & self.allowed_targets[knight.first_piece_square().unwrap() as usize];

            Self::targets_to_moves(knight, Piece::Knight, targets, board, moves);
        }
    }
    fn generate_knight_attacks(board: &Board, attacks_from_square: &mut [Bitboard; 64]) {
        let mut knights = board.knights(board.color_to_move());
        while let Some(knight) = knights.bitscan() {
            attacks_from_square[knight.first_piece_index().unwrap() as usize] |=
                Self::generate_knight_targets(knight);
        }
    }
    fn generate_pawn_moves(&self, board: &Board, moves: &mut Vec<Move>) {
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

        let mut pawns = board.pawns(board.color_to_move());
        while let Some(pawn) = pawns.bitscan() {
            let pawn_index = pawn.first_piece_square().unwrap() as usize;

            let targets = pawn.shift(forwards) & !board.all_piece_bitboard();
            let mut double_targets = (targets & shifted_baseline).shift(forwards)
                & !board.all_piece_bitboard()
                & self.allowed_targets[pawn_index];

            let targets = targets & self.allowed_targets[pawn_index];

            let attack_targets = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            let attacks = attack_targets
                & board.get_color_bitboard(board.color_to_move().opposite())
                & self.allowed_targets[pawn_index];

            let mut promotion_targets = targets & promotion_line;
            let mut normal_targets = targets & !promotion_line;

            let mut promotion_attacks = attacks & promotion_line;
            let mut normal_attacks = attacks & !promotion_line;

            if ALL_MOVES {
                while let Some(target) = normal_targets.bitscan() {
                    moves.push(Move::Normal {
                        from_square: pawn,
                        to_square: target,
                        captured_piece: None,
                        moved_piece: Piece::Pawn,
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
                            captured_piece: None,
                        });
                    }
                }
            }

            // en passant
            if let Some(target) = (attack_targets & board.enpassant_square()).bitscan()
                && let captured_pawn = target.wrapping_shift(forwards.opposite(), 1)
                && !(self.allowed_targets[pawn.first_piece_index().unwrap() as usize]
                    & captured_pawn)
                    .is_empty()
            {
                let king = board.king(board.color_to_move());

                let king_square = king.first_piece_square().unwrap();
                let pawn_square = pawn.first_piece_square().unwrap();

                if king_square.row() == pawn_square.row() {
                    // maybe we have both pawns pinned to the king

                    let both_pawns = captured_pawn | pawn;

                    let sliders = board.rooks(board.color_to_move().opposite())
                        | board.queens(board.color_to_move().opposite());

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
                    captured_piece: Some(board.get_piece_from_bitboard(target).unwrap()),
                    moved_piece: Piece::Pawn,
                })
            }

            while let Some(target) = promotion_attacks.bitscan() {
                for promotion_piece in [Piece::Bishop, Piece::Knight, Piece::Rook, Piece::Queen] {
                    moves.push(Move::Promotion {
                        from_square: pawn,
                        to_square: target,
                        promotion_piece,
                        captured_piece: Some(board.get_piece_from_bitboard(target).unwrap()),
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

        let mut pawns = board.pawns(board.color_to_move());
        while let Some(pawn) = pawns.bitscan() {
            let attacks = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            attacks_from_square[pawn.first_piece_index().unwrap() as usize] |= attacks;
        }
    }

    fn magic_mask_for_rook(piece: Bitboard) -> Bitboard {
        let vertical_mask =
            Self::sliding_moves(piece, [Direction::North, Direction::South], Bitboard(0));

        let vertical_mask = vertical_mask
            & bitboard!(
                0b_00000000
                0b_11111111
                0b_11111111
                0b_11111111
                0b_11111111
                0b_11111111
                0b_11111111
                0b_00000000
            );

        let horizontal_mask =
            Self::sliding_moves(piece, [Direction::East, Direction::West], Bitboard(0));

        let horizontal_mask = horizontal_mask
            & bitboard!(
                0b_01111110
                0b_01111110
                0b_01111110
                0b_01111110
                0b_01111110
                0b_01111110
                0b_01111110
                0b_01111110
            );

        vertical_mask | horizontal_mask
    }
    fn magic_mask_for_bishop(piece: Bitboard) -> Bitboard {
        let mask = Self::sliding_moves(
            piece,
            [
                Direction::NorthEast,
                Direction::SouthEast,
                Direction::SouthWest,
                Direction::NorthWest,
            ],
            Bitboard(0),
        );

        mask & bitboard!(
            0b_00000000
            0b_01111110
            0b_01111110
            0b_01111110
            0b_01111110
            0b_01111110
            0b_01111110
            0b_00000000
        )
    }

    fn occupancies_for_mask(mut mask: Bitboard) -> impl Iterator<Item = Bitboard> {
        let num_bits = mask.0.count_ones();
        let num_occupancies = 1 << num_bits;

        let mut indices = vec![];

        while let Some(index) = mask.bitscan_index() {
            indices.push(index);
        }

        (0..num_occupancies).map(move |occupancy| {
            let mut occupancy_bitboard = Bitboard(0);

            for (index_i, index) in indices.iter().enumerate() {
                let selected_bit = (occupancy >> index_i) & 0b1;
                occupancy_bitboard |= Bitboard(selected_bit << index);
            }

            occupancy_bitboard
        })
    }

    pub fn generate_magic_entry(
        square: Bitboard,
        magic: u64,
        piece: Piece,
    ) -> Option<(MagicTableEntry, u64)> {
        let mask = match piece {
            Piece::Rook => Self::magic_mask_for_rook(square),
            Piece::Bishop => Self::magic_mask_for_bishop(square),
            _ => unimplemented!(),
        };
        let occupancies = Self::occupancies_for_mask(mask);

        let bits_used = match piece {
            Piece::Rook => 12,
            Piece::Bishop => 9,
            _ => unimplemented!(),
        };

        let mut table = vec![false; 1 << bits_used];

        let mut max_entry = 0;

        for occupancy in occupancies {
            let index = (occupancy.0.wrapping_mul(magic)) >> (64 - bits_used);
            if table[index as usize] {
                return None;
            }
            table[index as usize] = true;

            max_entry = max_entry.max(index);
        }

        Some((
            MagicTableEntry {
                magic,
                mask,
                bits_used,
            },
            max_entry,
        ))
    }

    pub fn generate_magic_table(
        square: u32,
        magic_entry: MagicTableEntry,
        piece_type: Piece,
        table_size: usize,
    ) -> Vec<Bitboard> {
        let piece = Bitboard(1 << square);

        let mask = match piece_type {
            Piece::Rook => Self::magic_mask_for_rook(piece),
            Piece::Bishop => Self::magic_mask_for_bishop(piece),
            _ => unimplemented!(),
        };
        let occupancies = Self::occupancies_for_mask(mask);

        let mut out = vec![Bitboard(0); table_size];

        for occupancy in occupancies {
            let index = magic_entry.apply(occupancy);
            out[index as usize] = Self::sliding_moves(
                piece,
                match piece_type {
                    Piece::Rook => [
                        Direction::North,
                        Direction::East,
                        Direction::South,
                        Direction::West,
                    ],
                    Piece::Bishop => [
                        Direction::NorthEast,
                        Direction::NorthWest,
                        Direction::SouthEast,
                        Direction::SouthWest,
                    ],
                    _ => unimplemented!(),
                },
                occupancy,
            );
        }

        out
    }
}
pub const ROOK_MAGIC_VALUES: [u64; 64] = [
    11637302811523555600,
    4507997808111744,
    9042400808798208,
    4503668382565520,
    9027128523816992,
    74313791931686920,
    18015498562177152,
    36050935427907840,
    2305913382590480705,
    2305879310294061056,
    1225049501748969489,
    5188181981946708000,
    9655753352392212744,
    4611721340255211648,
    4611756389319065601,
    9223407221763752066,
    90072129988464640,
    5629533961062912,
    563018673162244,
    4938197128917811712,
    1125968899015168,
    144256063013654784,
    9148005733105728,
    5926877984551075872,
    72163147691069440,
    4505798784846848,
    324263571485753425,
    10664528316198814208,
    283674538934528,
    4611758586464305328,
    9263904984330682369,
    5801764421399781377,
    563087409676688,
    1152930317880787024,
    288798007620534416,
    4620702015926894656,
    432486306020343824,
    1752463209297625120,
    667693831279345728,
    144260461051772993,
    4611721203067979783,
    9225623905455050760,
    4719842846964728332,
    4612249518674542848,
    117130149106827268,
    1162212377869950977,
    1190077094894256129,
    288230651063246852,
    4612829511061471264,
    288379926914039824,
    9512166462622793736,
    568722930663456,
    9009400962547744,
    288230926444462112,
    9008298774757440,
    8072719941431984392,
    144255925835153665,
    38298468596613130,
    576478481928946753,
    2486004878690754562,
    2684162987312807937,
    612491782714032129,
    277126054913,
    720721359986037890,
];
pub const BISHOP_MAGIC_VALUES: [u64; 64] = [
    70920655865920,
    141287378919556,
    4683884488466497792,
    141149872390160,
    74775380886528,
    612560201538732032,
    576479448312857088,
    9223442551644651680,
    144119590419433488,
    2314991222984279072,
    603482899961874440,
    282025941533840,
    1158058715023163584,
    563229134700544,
    5800680575941738560,
    1161937570823733280,
    36319069196025872,
    9223444880045383746,
    10376575018602661890,
    1152956697577263108,
    306387161685688832,
    9015996492939392,
    49548393072050215,
    6920911469275021336,
    72622743031390241,
    18155687903297554,
    9223654065899047168,
    578721348245913632,
    288511919864692738,
    18049859915366432,
    288248037061443604,
    146437373814120462,
    288512951780933764,
    144186656902103106,
    4071395351603479040,
    468449213937091088,
    5633914760728832,
    1227231998108516384,
    9259541708952059912,
    72198400312872964,
    9078669666418752,
    35735207149600,
    2360036017500586112,
    9223377020092678208,
    2200098054400,
    288301295192768640,
    721139440625655817,
    140875063640066,
    1441170024915028112,
    36099441718993924,
    9223372071759798528,
    81064793431093312,
    39586715672578,
    2380152957117145092,
    1153062381690032128,
    90635217649271424,
    36100266365321356,
    1157566117139513376,
    9042487110828129,
    9836988603044479024,
    2313161358624956450,
    2310364218211205129,
    4402345676804,
    140879239061512,
];

fn magic_entry_generator(piece: Piece, magic_values: &[u64; 64]) -> [MagicTableEntry; 64] {
    let mut entries = [MagicTableEntry {
        magic: 0,
        mask: Bitboard(0),
        bits_used: 0,
    }; 64];

    for square in 0..64 {
        entries[square] = MoveGenerator::<true>::generate_magic_entry(
            Bitboard(1 << square),
            magic_values[square],
            piece,
        )
        .unwrap_or_else(|| panic!("The {piece:?} magic values aren't unique"))
        .0;
    }

    entries
}
fn magic_entry_size_generator(piece: Piece, magic_values: &[u64; 64]) -> [usize; 64] {
    let mut entries = [0; 64];

    for square in 0..64 {
        entries[square] = MoveGenerator::<true>::generate_magic_entry(
            Bitboard(1 << square),
            magic_values[square],
            piece,
        )
        .unwrap_or_else(|| panic!("The {piece:?} magic values aren't unique"))
        .1 as usize
            + 1;
    }

    entries
}

pub static ROOK_MAGIC_ENTRIES: LazyLock<[MagicTableEntry; 64]> =
    LazyLock::new(|| magic_entry_generator(Piece::Rook, &ROOK_MAGIC_VALUES));
pub static ROOK_MAGIC_TABLE_SIZES: LazyLock<[usize; 64]> =
    LazyLock::new(|| magic_entry_size_generator(Piece::Rook, &ROOK_MAGIC_VALUES));
pub static ROOK_MAGIC_TABLE: LazyLock<[Vec<Bitboard>; 64]> = LazyLock::new(|| {
    (0..64)
        .map(|i| {
            MoveGenerator::<true>::generate_magic_table(
                i,
                ROOK_MAGIC_ENTRIES[i as usize],
                Piece::Rook,
                ROOK_MAGIC_TABLE_SIZES[i as usize],
            )
        })
        .collect_array()
        .unwrap()
});

pub static BISHOP_MAGIC_ENTRIES: LazyLock<[MagicTableEntry; 64]> =
    LazyLock::new(|| magic_entry_generator(Piece::Bishop, &BISHOP_MAGIC_VALUES));
pub static BISHOP_MAGIC_TABLE_SIZES: LazyLock<[usize; 64]> =
    LazyLock::new(|| magic_entry_size_generator(Piece::Bishop, &BISHOP_MAGIC_VALUES));
pub static BISHOP_MAGIC_TABLE: LazyLock<[Vec<Bitboard>; 64]> = LazyLock::new(|| {
    (0..64)
        .map(|i| {
            MoveGenerator::<true>::generate_magic_table(
                i,
                BISHOP_MAGIC_ENTRIES[i as usize],
                Piece::Bishop,
                BISHOP_MAGIC_TABLE_SIZES[i as usize],
            )
        })
        .collect_array()
        .unwrap()
});
