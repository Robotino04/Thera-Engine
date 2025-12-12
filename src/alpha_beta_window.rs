use crate::{centi_pawns::Evaluation, transposition_table::EvalKind};

/// Doesn't derive Copy, because accidentally copying
/// the search window will cause awful bugs.
#[derive(Debug, Clone)]
pub struct AlphaBetaWindow {
    starting_alpha: Evaluation,
    alpha: Evaluation,
    beta: Evaluation,
    best: Evaluation,
    plies: u32,
}

#[derive(Debug)]
pub enum WindowUpdate {
    NoImprovement,
    NewBest,
    Prune,
}

#[derive(Debug, Clone, Copy)]
pub struct NodeEvalSummary {
    pub eval: Evaluation,
    pub kind: EvalKind,
    pub plies: u32,
}

impl Default for AlphaBetaWindow {
    fn default() -> Self {
        Self::new(Evaluation::MIN, Evaluation::MAX, 0)
    }
}

impl AlphaBetaWindow {
    pub fn new(alpha: Evaluation, beta: Evaluation, plies: u32) -> Self {
        assert!(alpha <= beta, "alpha: {alpha:?}\nbeta: {beta:?}");
        Self {
            alpha,
            starting_alpha: alpha,
            beta,
            best: Evaluation::MIN,
            plies,
        }
    }

    #[must_use = "You should always handle potential beta-cutoffs"]
    pub fn update(&mut self, eval: Evaluation) -> WindowUpdate {
        if eval > self.alpha {
            self.alpha = eval;
        }
        if eval > self.best {
            self.best = eval;

            if self.has_cutoff() {
                WindowUpdate::Prune
            } else {
                WindowUpdate::NewBest
            }
        } else {
            WindowUpdate::NoImprovement
        }
    }

    pub fn next_depth(&self) -> Self {
        let new_alpha = -self.beta;
        let new_beta = -self.alpha;
        let new_plies = self.plies + 1;

        Self::new(new_alpha, new_beta, new_plies)
    }

    #[must_use = "Providing an exact node value and not using it is likely a mistake"]
    pub fn set_exact(self, eval: Evaluation) -> NodeEvalSummary {
        NodeEvalSummary {
            eval,
            kind: EvalKind::Exact,
            plies: self.plies,
        }
    }

    pub fn causes_cutoff(&self, eval: Evaluation) -> bool {
        eval >= self.beta
    }

    fn has_cutoff(&self) -> bool {
        self.causes_cutoff(self.best)
    }

    pub fn fails_low(&self, eval: Evaluation) -> bool {
        eval <= self.alpha
    }

    pub fn plies(&self) -> u32 {
        self.plies
    }

    #[must_use = "Finalizing a search without using the results is likely a mistake"]
    pub fn finalize(self) -> NodeEvalSummary {
        NodeEvalSummary {
            eval: self.best,
            kind: if self.best <= self.starting_alpha {
                EvalKind::UpperBound
            } else if self.best >= self.beta {
                EvalKind::LowerBound
            } else {
                EvalKind::Exact
            },
            plies: self.plies,
        }
    }
}
