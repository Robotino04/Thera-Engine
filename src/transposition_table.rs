use crate::{board::Board, centi_pawns::Evaluation};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum EvalKind {
    Exact,
    LowerBound,
    UpperBound,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct TranspositionEntry {
    pub hash: u64,
    pub eval: Evaluation,
    pub depth: u32,
    pub kind: EvalKind,
    pub subnodes: u64,
}

#[derive(Debug, Clone)]
pub struct TranspositionTable {
    table: Vec<Option<TranspositionEntry>>,
    used_slots: usize,
}

impl TranspositionTable {
    pub fn new(size: usize) -> Self {
        Self {
            table: vec![None; size],
            used_slots: 0,
        }
    }

    pub fn get(&self, board: &Board, depth: u32, plies: u32) -> Option<TranspositionEntry> {
        let hash_part = board.zobrist_hash() % self.table.len() as u64;
        let mut entry = self.table[hash_part as usize]?;
        if entry.hash == board.zobrist_hash() && entry.depth >= depth {
            entry.eval = match entry.eval {
                Evaluation::Win(x) => Evaluation::Win(x + plies),
                Evaluation::Loss(x) => Evaluation::Loss(x + plies),
                x @ Evaluation::CentiPawns(_) => x,
            };
            Some(entry)
        } else {
            None
        }
    }

    pub fn insert(
        &mut self,
        board: &Board,
        depth: u32,
        eval: Evaluation,
        kind: EvalKind,
        subnodes: u64,
        plies: u32,
    ) {
        let hash_part = board.zobrist_hash() % self.table.len() as u64;
        let entry = &mut self.table[hash_part as usize];
        let replace =
            entry.is_none_or(|entry| depth >= entry.depth || entry.hash != board.zobrist_hash());
        if replace {
            if entry.is_none() {
                self.used_slots += 1;
            }
            *entry = Some(TranspositionEntry {
                hash: board.zobrist_hash(),
                eval: match eval {
                    Evaluation::Win(x) => Evaluation::Win(x - plies),
                    Evaluation::Loss(x) => Evaluation::Loss(x - plies),
                    x @ Evaluation::CentiPawns(_) => x,
                },
                depth,
                kind,
                subnodes,
            });
        }
    }

    pub fn capacity(&self) -> usize {
        self.table.len()
    }
    pub fn used_slots(&self) -> usize {
        self.used_slots
    }
}
