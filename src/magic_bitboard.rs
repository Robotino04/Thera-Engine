use std::sync::LazyLock;

use itertools::Itertools;

use crate::{
    bitboard::{Bitboard, bitboard},
    move_generator::occluded_fill,
    piece::{BySquare, Direction, Piece, Square},
};

#[derive(Clone, Copy, Debug, Default)]
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

pub fn rook_magic_bitboard(from_square: Square, occupancy: Bitboard) -> Bitboard {
    ROOK_MAGIC_TABLE[from_square][ROOK_MAGIC_ENTRIES[from_square].apply(occupancy) as usize]
}
pub fn bishop_magic_bitboard(from_square: Square, occupancy: Bitboard) -> Bitboard {
    BISHOP_MAGIC_TABLE[from_square][BISHOP_MAGIC_ENTRIES[from_square].apply(occupancy) as usize]
}

fn sliding_moves<const N: usize>(
    piece: Bitboard,
    directions: [Direction; N],
    blockers: Bitboard,
) -> Bitboard {
    let mut targets = Bitboard::default();
    for dir in directions {
        let non_attacks = occluded_fill(piece, blockers, dir);
        targets |= non_attacks.shift(dir);
    }
    targets
}

fn magic_mask_for_rook(piece: Bitboard) -> Bitboard {
    let vertical_mask = sliding_moves(piece, [Direction::North, Direction::South], Bitboard(0));

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

    let horizontal_mask = sliding_moves(piece, [Direction::East, Direction::West], Bitboard(0));

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
    let mask = sliding_moves(
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
        Piece::Rook => magic_mask_for_rook(square),
        Piece::Bishop => magic_mask_for_bishop(square),
        _ => unimplemented!(),
    };
    let occupancies = occupancies_for_mask(mask);

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
    square: Square,
    magic_entry: MagicTableEntry,
    piece_type: Piece,
    table_size: usize,
) -> Vec<Bitboard> {
    let piece = Bitboard::from_square(square);

    let mask = match piece_type {
        Piece::Rook => magic_mask_for_rook(piece),
        Piece::Bishop => magic_mask_for_bishop(piece),
        _ => unimplemented!(),
    };
    let occupancies = occupancies_for_mask(mask);

    let mut out = vec![Bitboard(0); table_size];

    for occupancy in occupancies {
        let index = magic_entry.apply(occupancy);
        out[index as usize] = sliding_moves(
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

pub const ROOK_MAGIC_VALUES: BySquare<u64> = BySquare::new([
    11637302811523555600,
    9007267974742048,
    9241404027684536324,
    18018797630259202,
    18018797629865985,
    9007749547491848,
    18014673395777792,
    36050935427907840,
    1152930369960477696,
    108104000423329810,
    1369103100027142152,
    4611721340305539584,
    36063989983084800,
    35188669153408,
    4611756389319065601,
    35184908976257,
    9241386572804259880,
    289356344778555408,
    4612811952710877192,
    577586789682782212,
    4621256305091289090,
    281543964655680,
    9148005733105728,
    5926877984551075872,
    18049583150530576,
    12393914970885324816,
    73184593590747144,
    10664528316198814208,
    18016598071705856,
    18015498560077952,
    360358340007591937,
    35184908976257,
    5782621990265290784,
    4505815831019528,
    4612814121652977672,
    35201554055680,
    72620548288421890,
    184682773391343744,
    667693831279345728,
    11097010356770308160,
    5206196422398969856,
    4512400048915456,
    2450028601475613184,
    162133984665411616,
    13512997922152480,
    2533412237746177,
    5194972541063544833,
    4612812503590650882,
    162129655306911776,
    8796361719824,
    1154047438890336264,
    2449993390251835424,
    945757021796433952,
    288230926444462112,
    35185445863456,
    140874931503136,
    144255925835153665,
    2378182215802585153,
    1189091176688387138,
    2017617306122719234,
    2684162987312807937,
    36033229459062785,
    2314850767888057345,
    720721359986037890,
]);
pub const BISHOP_MAGIC_VALUES: BySquare<u64> = BySquare::new([
    70920655865920,
    141287378919556,
    4683884488466497792,
    141149872390160,
    74775380886528,
    36065909839757314,
    576479448312857088,
    18033094506446912,
    144119590419433488,
    2314991222984279072,
    603482899961874440,
    282025941533840,
    1158058715023163584,
    4503744599329794,
    5800680575941738560,
    1161937570823733280,
    36319069196025872,
    9223444880045383746,
    36099174386632736,
    1152956697577263108,
    4755841340833792004,
    1188959099874967680,
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
    576601768968880256,
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
    648588990567481408,
    1157566117139513376,
    22658735894724704,
    9836988603044479024,
    2313161358624956450,
    2310364218211205129,
    4402345676804,
    140879239061512,
]);

fn magic_entry_generator(piece: Piece, magic_values: &BySquare<u64>) -> BySquare<MagicTableEntry> {
    let mut entries: BySquare<_> = Default::default();

    for square in Square::ALL {
        entries[square] =
            generate_magic_entry(Bitboard::from_square(square), magic_values[square], piece)
                .unwrap_or_else(|| panic!("The {piece:?} magic values aren't unique"))
                .0;
    }

    entries
}
fn magic_entry_size_generator(piece: Piece, magic_values: &BySquare<u64>) -> BySquare<usize> {
    let mut entries: BySquare<_> = Default::default();

    for square in Square::ALL {
        entries[square] =
            generate_magic_entry(Bitboard::from_square(square), magic_values[square], piece)
                .unwrap_or_else(|| panic!("The {piece:?} magic values aren't unique"))
                .1 as usize
                + 1;
    }

    entries
}

pub static ROOK_MAGIC_ENTRIES: LazyLock<BySquare<MagicTableEntry>> =
    LazyLock::new(|| magic_entry_generator(Piece::Rook, &ROOK_MAGIC_VALUES));
pub static ROOK_MAGIC_TABLE_SIZES: LazyLock<BySquare<usize>> =
    LazyLock::new(|| magic_entry_size_generator(Piece::Rook, &ROOK_MAGIC_VALUES));
pub static ROOK_MAGIC_TABLE: LazyLock<BySquare<Vec<Bitboard>>> = LazyLock::new(|| {
    Square::ALL
        .into_iter()
        .map(|square| {
            generate_magic_table(
                square,
                ROOK_MAGIC_ENTRIES[square],
                Piece::Rook,
                ROOK_MAGIC_TABLE_SIZES[square],
            )
        })
        .collect_array()
        .unwrap()
        .into()
});

pub static BISHOP_MAGIC_ENTRIES: LazyLock<BySquare<MagicTableEntry>> =
    LazyLock::new(|| magic_entry_generator(Piece::Bishop, &BISHOP_MAGIC_VALUES));
pub static BISHOP_MAGIC_TABLE_SIZES: LazyLock<BySquare<usize>> =
    LazyLock::new(|| magic_entry_size_generator(Piece::Bishop, &BISHOP_MAGIC_VALUES));
pub static BISHOP_MAGIC_TABLE: LazyLock<BySquare<Vec<Bitboard>>> = LazyLock::new(|| {
    Square::ALL
        .into_iter()
        .map(|square| {
            generate_magic_table(
                square,
                BISHOP_MAGIC_ENTRIES[square],
                Piece::Bishop,
                BISHOP_MAGIC_TABLE_SIZES[square],
            )
        })
        .collect_array()
        .unwrap()
        .into()
});
