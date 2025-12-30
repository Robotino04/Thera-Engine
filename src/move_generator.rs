use std::fmt::Display;

use crate::{
    bitboard::{Bitboard, bitboard},
    board::Board,
    color::Color,
    direction::Direction,
    magic_bitboard::{bishop_magic_bitboard, rook_magic_bitboard},
    piece::Piece,
    square::{BySquare, Square},
};

#[derive(Debug)]
pub struct MoveGenerator {
    #[expect(dead_code)]
    attacks_from_square: BySquare<Bitboard>,
    attacks_to_square: BySquare<Bitboard>,
    /// NOTE: this does not consider the king as a blocker for sliding pieces.
    /// That's a good thing for king move generation, but maybe not expected for other uses
    attacked_squares: Bitboard,
    allowed_targets: BySquare<Bitboard>,
    is_double_check: bool,
    is_check: bool,
}

const MAX_MOVES_IN_POSITION: usize = 218;

/// https://www.chessprogramming.org/Kogge-Stone_Algorithm
pub fn occluded_fill(mut seeds: Bitboard, blockers: Bitboard, dir: Direction) -> Bitboard {
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

impl MoveGenerator {
    /// https://www.chessprogramming.org/Square_Attacked_By#Pure_Calculation
    pub const SQUARES_IN_BETWEEN: BySquare<BySquare<Bitboard>> = const {
        let mut tmp = BySquare::new([BySquare::new([Bitboard(0); Square::COUNT]); Square::COUNT]);
        let mut sq1 = 0;
        while sq1 < Square::COUNT {
            let mut sq2 = 0;
            while sq2 < Square::COUNT {
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

                tmp.const_as_mut()[sq1].const_as_mut()[sq2] = Bitboard(line & btwn); /* return the bits on that line in-between */

                sq2 += 1;
            }
            sq1 += 1;
        }

        tmp
    };

    pub fn with_attacks(board: &mut Board) -> Self {
        let mut attacks_from_square = BySquare::default();

        {
            let board = board.make_null_move();

            Self::generate_king_attacks(&board, &mut attacks_from_square);
            Self::generate_bishop_attacks(&board, &mut attacks_from_square);
            Self::generate_knight_attacks(&board, &mut attacks_from_square);
            Self::generate_rook_attacks(&board, &mut attacks_from_square);
            Self::generate_queen_attacks(&board, &mut attacks_from_square);
            Self::generate_pawn_attacks(&board, &mut attacks_from_square);
        }

        let mut attacks_to_square = BySquare::default();
        let mut attacked_squares = Bitboard(0);
        for (i, bb) in attacks_from_square.as_ref().iter().cloned().enumerate() {
            attacked_squares |= bb;
            for target_index in bb.squares() {
                attacks_to_square[target_index] |= Bitboard(1 << i);
            }
        }

        let king = board.king(board.color_to_move());
        let king_square = king.first_piece_square().unwrap_or_else(|| {
            let current_fen = board.to_fen();
            unreachable!("{current_fen}: position doesn't have a king");
        });
        let king_attackers: Bitboard = attacks_to_square[king_square];

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
                Self::SQUARES_IN_BETWEEN[king_square][king_attackers.first_piece_square().unwrap()]
                    | king_attackers,
            )
        };

        let blockers = board.all_piece_bitboard();

        let mut target_restrictions = BySquare::new([allowed_targets; Square::COUNT]);
        for dir in Direction::ALL {
            let pinned_squares = Self::pinned_pieces(board, blockers, king, dir);
            let pinned_squares2 = pinned_squares;
            for square in pinned_squares2.squares() {
                target_restrictions[square] &= pinned_squares;
            }
        }

        let attacked_squares = attacks_from_square
            .as_ref()
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
        self.generate_moves_for_all_pieces::<true>(board)
    }
    pub fn generate_captures(&self, board: &Board) -> Vec<Move> {
        self.generate_moves_for_all_pieces::<false>(board)
    }

    pub fn generate_moves_for_all_pieces<const ALL_MOVES: bool>(&self, board: &Board) -> Vec<Move> {
        if !self.is_double_check {
            let mut moves = Vec::with_capacity(MAX_MOVES_IN_POSITION);

            self.generate_pawn_moves::<ALL_MOVES>(board, &mut moves);
            self.generate_bishop_moves::<ALL_MOVES>(board, &mut moves);
            self.generate_knight_moves::<ALL_MOVES>(board, &mut moves);
            self.generate_rook_moves::<ALL_MOVES>(board, &mut moves);
            self.generate_queen_moves::<ALL_MOVES>(board, &mut moves);
            self.generate_king_moves::<ALL_MOVES>(board, &mut moves);

            moves
        } else {
            let mut moves = Vec::with_capacity(8);

            self.generate_king_moves::<ALL_MOVES>(board, &mut moves);

            moves
        }
    }

    pub fn is_check(&self) -> bool {
        self.is_check
    }
    pub fn is_double_check(&self) -> bool {
        self.is_double_check
    }
    pub fn attacked_squares(&self) -> Bitboard {
        self.attacked_squares
    }

    pub fn least_valuable_attacker(&self, board: &Board, square: Square) -> Option<Piece> {
        let attackers = self.attacks_to_square[square];
        [
            Piece::Pawn,
            Piece::Knight,
            Piece::Bishop,
            Piece::Rook,
            Piece::Queen,
            Piece::King,
        ]
        .into_iter()
        .find(|&piece| !(board.get_piece_bitboard_colorless(piece) & attackers).is_empty())
    }

    fn pinned_pieces(
        board: &Board,
        mut blockers: Bitboard,
        piece: Bitboard,
        dir: Direction,
    ) -> Bitboard {
        let targets = occluded_fill(piece, blockers, dir).shift(dir);

        let attacks = targets & blockers;
        blockers ^= attacks; // remove one line of blockers

        let xrayed_squares = occluded_fill(piece, blockers, dir).shift(dir);

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

    fn targets_to_moves<const ALL_MOVES: bool>(
        origin: Bitboard,
        moved_piece: Piece,
        targets: Bitboard,
        board: &Board,
        moves: &mut Vec<Move>,
    ) {
        if ALL_MOVES {
            let non_captures = targets & !board.all_piece_bitboard();

            for target in non_captures {
                moves.push(Move::Normal {
                    from_square: origin,
                    to_square: target,
                    captured_piece: None,
                    moved_piece,
                });
            }
        }

        let captures = targets & board.get_color_bitboard(board.color_to_move().opposite());

        for target in captures {
            moves.push(Move::Normal {
                from_square: origin,
                to_square: target,
                captured_piece: Some(board.get_piece_from_bitboard(target).unwrap()),
                moved_piece,
            });
        }
    }

    fn generate_king_targets(king: Bitboard) -> Bitboard {
        let middle_row = king.shift(Direction::West) | king | king.shift(Direction::East);
        let targets =
            middle_row.shift(Direction::North) | middle_row | middle_row.shift(Direction::South);

        targets ^ king
    }

    fn generate_king_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        let king = board.king(board.color_to_move());
        let targets = Self::generate_king_targets(king);
        attacks_from_square[king.first_piece_square().unwrap()] |= targets;
    }

    fn generate_king_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
        let king = board.king(board.color_to_move());
        let targets = Self::generate_king_targets(king) & !self.attacked_squares;

        Self::targets_to_moves::<ALL_MOVES>(king, Piece::King, targets, board, moves);

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

    fn generate_rook_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
        for rook in board.rooks(board.color_to_move()).squares() {
            let occupancy = board.all_piece_bitboard();

            let targets = rook_magic_bitboard(rook, occupancy)
                & self.allowed_targets[rook]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves::<ALL_MOVES>(
                Bitboard::from_square(rook),
                Piece::Rook,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_rook_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        for rook in board.rooks(board.color_to_move()).squares() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            attacks_from_square[rook] |= rook_magic_bitboard(rook, occupancy);
        }
    }
    fn generate_bishop_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
        for bishops in board.bishops(board.color_to_move()).squares() {
            let occupancy = board.all_piece_bitboard();
            let targets = bishop_magic_bitboard(bishops, occupancy)
                & self.allowed_targets[bishops]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves::<ALL_MOVES>(
                Bitboard::from_square(bishops),
                Piece::Bishop,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_bishop_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        for bishop in board.bishops(board.color_to_move()).squares() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            let entry = bishop_magic_bitboard(bishop, occupancy);

            attacks_from_square[bishop] |= entry;
        }
    }

    fn generate_queen_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
        for queen in board.queens(board.color_to_move()).squares() {
            let occupancy = board.all_piece_bitboard();
            let targets = (bishop_magic_bitboard(queen, occupancy)
                | rook_magic_bitboard(queen, occupancy))
                & self.allowed_targets[queen]
                & !board.get_color_bitboard(board.color_to_move());

            Self::targets_to_moves::<ALL_MOVES>(
                Bitboard::from_square(queen),
                Piece::Queen,
                targets,
                board,
                moves,
            );
        }
    }
    fn generate_queen_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        for queen in board.queens(board.color_to_move()).squares() {
            let occupancy =
                board.all_piece_bitboard() ^ board.king(board.color_to_move().opposite());
            let rook_entry = rook_magic_bitboard(queen, occupancy);
            let bishop_entry = bishop_magic_bitboard(queen, occupancy);

            attacks_from_square[queen] |= rook_entry | bishop_entry;
        }
    }

    fn generate_knight_targets(knight: Bitboard) -> Bitboard {
        const LUT: [Bitboard; Square::COUNT] = {
            let mut lut = [Bitboard(0); Square::COUNT];

            let mut square = 0;
            while square < Square::COUNT {
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

                lut[square] = targets;

                square += 1;
            }

            lut
        };

        LUT[knight.first_piece_index().unwrap() as usize]
    }

    fn generate_knight_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
        for knight in board.knights(board.color_to_move()) {
            let targets = Self::generate_knight_targets(knight)
                & self.allowed_targets[knight.first_piece_square().unwrap()];

            Self::targets_to_moves::<ALL_MOVES>(knight, Piece::Knight, targets, board, moves);
        }
    }
    fn generate_knight_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        for knight in board.knights(board.color_to_move()) {
            attacks_from_square[knight.first_piece_square().unwrap()] |=
                Self::generate_knight_targets(knight);
        }
    }
    fn generate_pawn_moves<const ALL_MOVES: bool>(&self, board: &Board, moves: &mut Vec<Move>) {
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

        for pawn in board.pawns(board.color_to_move()) {
            let pawn_square = pawn.first_piece_square().unwrap();

            let targets = pawn.shift(forwards) & !board.all_piece_bitboard();
            let double_targets = (targets & shifted_baseline).shift(forwards)
                & !board.all_piece_bitboard()
                & self.allowed_targets[pawn_square];

            let targets = targets & self.allowed_targets[pawn_square];

            let attack_targets = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            let attacks = attack_targets
                & board.get_color_bitboard(board.color_to_move().opposite())
                & self.allowed_targets[pawn_square];

            let promotion_targets = targets & promotion_line;
            let normal_targets = targets & !promotion_line;

            let promotion_attacks = attacks & promotion_line;
            let normal_attacks = attacks & !promotion_line;

            if ALL_MOVES {
                for target in normal_targets {
                    moves.push(Move::Normal {
                        from_square: pawn,
                        to_square: target,
                        captured_piece: None,
                        moved_piece: Piece::Pawn,
                    })
                }
                for target in double_targets {
                    moves.push(Move::DoublePawn {
                        from_square: pawn,
                        to_square: target,
                    })
                }

                for target in promotion_targets {
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
            if let Some(target) = (attack_targets & board.enpassant_square()).first_piece()
                && let captured_pawn = target.wrapping_shift(forwards.opposite(), 1)
                && !(self.allowed_targets[pawn.first_piece_square().unwrap()] & captured_pawn)
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

                    let pin_line = if pawn_square.column() < king_square.column() {
                        occluded_fill(king, blockers, Direction::West).shift(Direction::West)
                    } else {
                        occluded_fill(king, blockers, Direction::East).shift(Direction::East)
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

            for target in normal_attacks {
                moves.push(Move::Normal {
                    from_square: pawn,
                    to_square: target,
                    captured_piece: Some(board.get_piece_from_bitboard(target).unwrap()),
                    moved_piece: Piece::Pawn,
                })
            }

            for target in promotion_attacks {
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
    fn generate_pawn_attacks(board: &Board, attacks_from_square: &mut BySquare<Bitboard>) {
        let forwards_west = match board.color_to_move() {
            Color::White => Direction::NorthWest,
            Color::Black => Direction::SouthWest,
        };
        let forwards_east = match board.color_to_move() {
            Color::White => Direction::NorthEast,
            Color::Black => Direction::SouthEast,
        };

        for pawn in board.pawns(board.color_to_move()) {
            let attacks = pawn.shift(forwards_west) | pawn.shift(forwards_east);

            attacks_from_square[pawn.first_piece_square().unwrap()] |= attacks;
        }
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Move {
    Normal {
        from_square: Bitboard,
        to_square: Bitboard,
        captured_piece: Option<Piece>,
        moved_piece: Piece,
    },
    DoublePawn {
        from_square: Bitboard,
        to_square: Bitboard,
    },
    EnPassant {
        from_square: Bitboard,
        to_square: Bitboard,
    },
    Castle {
        from_square: Bitboard,
        to_square: Bitboard,
        rook_from_square: Bitboard,
        rook_to_square: Bitboard,
    },
    Promotion {
        from_square: Bitboard,
        to_square: Bitboard,
        promotion_piece: Piece,
        captured_piece: Option<Piece>,
    },
}

impl Display for Move {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.to_algebraic())
    }
}

impl Move {
    pub fn to_algebraic(&self) -> String {
        match self {
            Move::Normal {
                from_square,
                to_square,
                captured_piece: _,
                moved_piece: _,
            }
            | Move::DoublePawn {
                from_square,
                to_square,
            }
            | Move::EnPassant {
                from_square,
                to_square,
            }
            | Move::Castle {
                from_square,
                to_square,
                rook_from_square: _,
                rook_to_square: _,
            } => {
                from_square
                    .first_piece_square()
                    .unwrap()
                    .to_algebraic()
                    .to_string()
                    + to_square.first_piece_square().unwrap().to_algebraic()
            }
            Move::Promotion {
                from_square,
                to_square,
                promotion_piece,
                captured_piece: _,
            } => {
                from_square
                    .first_piece_square()
                    .unwrap()
                    .to_algebraic()
                    .to_string()
                    + to_square.first_piece_square().unwrap().to_algebraic()
                    + promotion_piece.to_algebraic()
            }
        }
    }
}
