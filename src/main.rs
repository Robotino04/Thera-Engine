use std::io::{BufRead, BufReader, Write};

use clap::{ArgAction, Parser, Subcommand};

use itertools::Itertools;
use rustyline::DefaultEditor;
use rustyline::error::ReadlineError;
use thera::board::FenParseError;
use thera::perft::{PerftMove, perft};
use thera::{self, board::Board, move_generator::MoveGenerator};

fn perft_stockfish(fen: &str, depth: u32, preapplied_moves: &[String]) -> Vec<PerftMove> {
    use std::process::{Command, Stdio};

    let mut cmd = Command::new("stockfish")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        .unwrap();
    let mut stdin = cmd.stdin.as_ref().unwrap();
    if preapplied_moves.is_empty() {
        writeln!(stdin, "position fen {fen}").unwrap();
    } else {
        writeln!(
            stdin,
            "position fen {fen} moves {}",
            preapplied_moves.join(" ")
        )
        .unwrap();
    }
    writeln!(stdin, "go perft {depth}").unwrap();
    stdin.flush().unwrap();
    let stdout = BufReader::new(cmd.stdout.as_mut().unwrap());

    let stockfish_out = stdout
        .lines()
        .map(|l| l.unwrap())
        .take_while(|line| !line.starts_with("Nodes searched:"))
        .filter(|l| !(l.starts_with("info ") || l.starts_with("Stockfish") || l.trim().is_empty()))
        .map(|line| {
            let [algebraic_move, count] =
                line.split(":").map(|s| s.trim()).collect_array().unwrap();

            PerftMove {
                algebraic_move: algebraic_move.to_string(),
                nodes: count.parse().unwrap(),
            }
        })
        .collect_vec();

    cmd.wait().unwrap();

    stockfish_out
}

#[derive(Debug)]
struct ReplState {
    board: Board,
    always_print: bool,
}

#[derive(Debug, Parser)]
#[command(multicall = true, disable_help_flag = true)]
struct Repl {
    #[command(subcommand)]
    command: ReplCommands,
}

#[derive(Debug, Subcommand)]
#[command(disable_help_flag = true)]
enum ReplCommands {
    /// Runs a perft test on the current position. Outputs a breakdown of nodes for each move as
    /// well as the total number of nodes in the same format as stockfish.
    Perft {
        /// The number of plies to play before counting leaf nodes
        depth: u32,
    },
    /// Runs a perft test on the current position. Also feeds the position into stockfish. In case
    /// any moves aren't matching, one of the incorrect ones is played and another bisect is done
    /// on the new position. Reports the FEN string of the respective position as well as the series
    /// of move mismatches.
    Bisect {
        /// The number of plies to play before counting leaf nodes
        depth: u32,
    },
    /// Set up a position on the internal board.
    Position {
        #[command(subcommand)]
        subcmd: PositionCommand,
    },
    /// Prints the current board.
    Print {
        #[command(subcommand)]
        subcmd: Option<PrintSubcommand>,
    },
}

#[derive(Debug, Subcommand)]
#[command(disable_help_flag = true)]
enum PrintSubcommand {
    /// Configure the engine to print the board after command.
    Always {
        #[arg(action = ArgAction::Set)]
        /// Should printing be enabled or disabled.
        enable: bool,
    },
}

#[derive(Debug, Subcommand)]
#[command(disable_help_flag = true)]
enum PositionCommand {
    /// Loads the starting position for vanilla chess.
    Startpos {
        #[command(subcommand)]
        moves: Option<MovesSubcommand>,
    },
    /// Loads the position specified in the FEN string.
    Fen {
        #[clap(value_delimiter = ' ', num_args = 1..=6)]
        /// The FEN string to load.
        fen: Vec<String>,
        #[command(subcommand)]
        moves: Option<MovesSubcommand>,
    },
}

#[derive(Debug, Subcommand)]
#[command(disable_help_flag = true, disable_help_subcommand = true)]
enum MovesSubcommand {
    /// Applies a series of moves given in long algebraic notation after loading the position.
    Moves {
        /// The moves to apply after loading the position.
        moves: Vec<String>,
    },
}

fn repl_handle_line(state: &mut ReplState, line: &str) -> Result<bool, String> {
    let args = line.split_whitespace().collect_vec();
    let repl = Repl::try_parse_from(args).map_err(|e| e.to_string())?;
    match repl.command {
        ReplCommands::Perft { depth } => {
            repl_handle_perft(state, depth);
        }
        ReplCommands::Bisect { depth } => {
            repl_handle_bisect(state, depth);
        }
        ReplCommands::Position { subcmd } => {
            repl_handle_position(state, subcmd);
        }
        ReplCommands::Print { subcmd } => {
            repl_handle_print(state, subcmd);
        }
    }

    if state.always_print {
        println!("{}", state.board.dump_ansi(None));
    }

    Ok(false)
}

fn apply_moves(board: &mut Board, moves: Vec<String>) {
    for m in moves {
        let mut possible_moves = vec![];
        MoveGenerator::<true>::with_attacks(board).generate_all_moves(board, &mut possible_moves);

        if let Some(found_move) = possible_moves.iter().find(|m2| m2.to_algebraic() == m) {
            board.make_move(found_move);
        } else {
            println!(
                "The move {m} isn't possible in the current position.\
                All previous moves have been applied though."
            );
        }
    }
}

fn repl_handle_perft(state: &mut ReplState, depth: u32) {
    let perft_results = perft(&mut state.board, depth);
    for PerftMove {
        algebraic_move,
        nodes,
    } in perft_results.divide.iter().sorted()
    {
        println!("{algebraic_move}: {nodes}")
    }
    println!();
    println!("Nodes searched: {}", perft_results.node_count);
}
fn repl_handle_position(ReplState { board, .. }: &mut ReplState, cmd: PositionCommand) {
    match cmd {
        PositionCommand::Startpos { moves } => {
            *board = Board::starting_position();
            if let Some(MovesSubcommand::Moves { moves }) = moves {
                apply_moves(board, moves);
            }
        }
        PositionCommand::Fen { fen, moves } => {
            let fen = fen.join(" ");
            match Board::from_fen(&fen) {
                Ok(new_board) => {
                    *board = new_board;
                    if let Some(MovesSubcommand::Moves { moves }) = moves {
                        apply_moves(board, moves);
                    }
                }
                Err(err) => match err {
                    FenParseError::WrongPartCount => {
                        println!(
                            "This FEN is missing some components. You always need:\
                        \n - the pieces\
                        \n - the color to move (w/b)\
                        \n - castling rights (any combination of KQkq or - if no one can castle)\
                        \n - the en passant square (or - if en passant isn't possible)\
                        \n - the halfmove clock\
                        \n - the fullmove counter"
                        );
                    }
                    FenParseError::WrongRowCount => {
                        println!(
                            "There is an incorrect number of rows in this FEN. \
                        You always need exactly 8 rows separated slashes."
                        );
                    }
                    FenParseError::InvalidPiece(invalid_piece) => {
                        println!(
                            "There is no piece associated with the character '{invalid_piece}'.\
                        \nThese ones are available:\
                        \n - P/p: white/black pawn\
                        \n - B/b: white/black bishop\
                        \n - N/n: white/black knight\
                        \n - R/r: white/black rook\
                        \n - Q/q: white/black queen\
                        \n - K/k: white/black king"
                        );
                    }
                    FenParseError::TooManyPiecesInRow => {
                        println!(
                            "There are too many pieces in this FEN. \
                        You always need 8 rows separated by slashes. \
                        Each row must consist of exactly n pieces and spacing numbers adding up to (8 - n)"
                        );
                    }
                    FenParseError::InvalidTurnColor(c) => {
                        println!(
                            "There is no turn-color associated with the character '{c}'. Use either w (white) or b (black)."
                        );
                    }
                    FenParseError::InvalidCastlingRight(c) => {
                        println!(
                            "There is no castling right associated with the character '{c}'. \
                        Use any combination of these (or a dash if no one can castle):\
                        \n - K: White can castle on the kings side (o-o)\
                        \n - k: Black can castle on the kings side (o-o)\
                        \n - Q: White can castle on the queens side (o-o-o)\
                        \n - q: Black can castle on the queens side (o-o-o)"
                        );
                    }
                    FenParseError::InvalidEnpassantSquare => {
                        println!(
                            "The en passant square isn't valid. It should either be \
                        set to the square that the last double pawn move jumped \
                        over or, if the last move wasn't a pawn, a single dash"
                        );
                    }
                    FenParseError::InvalidHalfMoveClock => {
                        println!(
                            "The halfmove clock isn't valid. It should be the number of \
                        plies since the last pawn move, capture or castling change."
                        );
                    }
                    FenParseError::InvalidFullMoveCounter => {
                        println!(
                            "The fullmove counter isn't valid. It should be the number of \
                        moves since the start of the game."
                        );
                    }
                },
            }
        }
    }
}
fn repl_handle_print(state: &mut ReplState, cmd: Option<PrintSubcommand>) {
    if let Some(PrintSubcommand::Always { enable }) = cmd {
        state.always_print = enable;
    }
    if !state.always_print {
        println!("{}", state.board.dump_ansi(None));
    }
}

fn repl_handle_bisect(ReplState { board, .. }: &mut ReplState, depth: u32) {
    let fen = board.to_fen();
    let mut move_stack = vec![];

    'next_depth: for depth in (1..=depth).rev() {
        let movegen = MoveGenerator::<true>::with_attacks(board);
        let mut moves = Vec::new();
        movegen.generate_all_moves(board, &mut moves);

        let mut thera_out = perft(board, depth).divide;
        let mut stockfish_out = perft_stockfish(&fen, depth, &move_stack);
        thera_out.sort();
        stockfish_out.sort();

        for m in thera_out {
            if let Some(equiv) = stockfish_out
                .iter()
                .find(|m2| m.algebraic_move == m2.algebraic_move)
                .cloned()
            {
                stockfish_out.retain(|m2| m.algebraic_move != m2.algebraic_move);
                if m.nodes != equiv.nodes {
                    println!("{}", board.dump_ansi(None));
                    println!("{}", board.to_fen());
                    println!(
                        "Move {} has {} nodes for Thera, but {} for stockfish. Recusing into it",
                        m.algebraic_move, m.nodes, equiv.nodes
                    );
                    let m = moves
                        .iter()
                        .find(|actual_move| actual_move.to_algebraic() == m.algebraic_move)
                        .expect("Thera produced a move it doesn't know about");

                    move_stack.push(m.to_algebraic());
                    board.make_move(m);
                    continue 'next_depth;
                }
            } else {
                println!("{}", board.dump_ansi(None));
                println!("{}", board.to_fen());
                println!("Move {} is missing from stockfish", m.algebraic_move);
                return;
            }
        }
        if let Some(m) = stockfish_out.first() {
            println!("{}", board.dump_ansi(None));
            println!("{}", board.to_fen());
            println!("Move {} is missing from Thera", m.algebraic_move);
            return;
        }

        println!("All moves are identical");
        break;
    }
}

fn main() {
    let mut line_editor = DefaultEditor::new().unwrap();

    let mut state = ReplState {
        board: Board::starting_position(),
        always_print: false,
    };

    loop {
        match line_editor.readline("Thera〉") {
            Ok(line) => {
                let line = line.trim();
                if line.is_empty() {
                    continue;
                }
                line_editor.add_history_entry(line).unwrap();
                match repl_handle_line(&mut state, line) {
                    Ok(quit) => {
                        if quit {
                            break;
                        }
                    }
                    Err(err) => {
                        println!("{err}");
                    }
                }
            }
            Err(ReadlineError::Eof) => {
                break;
            }
            Err(err) => {
                println!("{err}");
            }
        }
    }
    // Explicit drop in case we ever place something below here.
    // This undoes any signal hooks so Ctrl-C works again.
    drop(line_editor);
}
