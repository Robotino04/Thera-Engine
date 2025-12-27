use crate::{
    alpha_beta_window::{AlphaBetaWindow, NodeEvalSummary},
    board::Board,
    centi_pawns::Evaluation,
    move_generator::Move,
};

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
    pub best_move: Option<Move>,
}

#[derive(Debug, Clone)]
pub struct TranspositionTable {
    table: Vec<Option<TranspositionEntry>>,
    used_slots: usize,
}

impl Default for TranspositionTable {
    fn default() -> Self {
        Self::new(1 << 20)
    }
}

impl TranspositionTable {
    pub fn new(size: usize) -> Self {
        Self {
            table: vec![None; size],
            used_slots: 0,
        }
    }

    pub fn get_if_improves(
        &self,
        board: &Board,
        depth: u32,
        window: &AlphaBetaWindow,
    ) -> Option<TranspositionEntry> {
        if let Some(mut entry) = self.get(board)
            && entry.depth >= depth
        {
            entry.eval = match entry.eval {
                Evaluation::Win(x) => Evaluation::Win(x + window.plies()),
                Evaluation::Loss(x) => Evaluation::Loss(x + window.plies()),
                x @ Evaluation::CentiPawns(_) => x,
            };

            // We have the adjusted entry, but it might not be relevant for this window
            let can_prune = match entry.kind {
                EvalKind::Exact => true,
                EvalKind::LowerBound => window.causes_cutoff(entry.eval),
                EvalKind::UpperBound => window.fails_low(entry.eval),
            };

            can_prune.then_some(entry)
        } else {
            None
        }
    }

    pub fn get(&self, board: &Board) -> Option<TranspositionEntry> {
        let hash_part = board.zobrist_hash() % self.table.len() as u64;
        let entry = self.table[hash_part as usize]?;
        if entry.hash == board.zobrist_hash() {
            Some(entry)
        } else {
            None
        }
    }

    pub fn insert(&mut self, board: &Board, depth: u32, node: NodeEvalSummary, subnodes: u64) {
        let hash = board.zobrist_hash();
        let hash_part = hash % self.table.len() as u64;
        let entry = &mut self.table[hash_part as usize];

        let replace = entry.is_none_or(|entry| depth >= entry.depth || entry.hash != hash);
        if replace {
            if entry.is_none() {
                self.used_slots += 1;
            }
            *entry = Some(TranspositionEntry {
                hash: board.zobrist_hash(),
                eval: match node.eval {
                    Evaluation::Win(x) => Evaluation::Win(x - node.plies),
                    Evaluation::Loss(x) => Evaluation::Loss(x - node.plies),
                    x @ Evaluation::CentiPawns(_) => x,
                },
                depth,
                kind: node.kind,
                subnodes,
                best_move: if let Some(entry) = entry
                    && entry.hash == hash
                {
                    // keep the previous best move if we somehow happen to get a deeper result
                    // without also finding a new best one
                    node.best_move.or(entry.best_move)
                } else {
                    node.best_move
                },
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
