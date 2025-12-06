use std::fmt::Write;
use std::io::{BufRead, BufReader, stdin, stdout};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::{Receiver, RecvError, SyncSender};
use std::sync::{Arc, mpsc};
use std::time::{Duration, Instant};

use atty::Stream;
use itertools::Itertools;

use rand::Rng;
use rustyline::error::ReadlineError;
use rustyline::{DefaultEditor, ExternalPrinter};

use thera::bitboard::Bitboard;
use thera::board::{FenParseError, UndoToken};
use thera::magic_bitboard::{
    BISHOP_MAGIC_VALUES, MagicTableEntry, ROOK_MAGIC_VALUES, generate_magic_entry,
};
use thera::perft::{PerftMove, PerftStatistics, perft, perft_nostats};
use thera::piece::{Move, Piece, Square};
use thera::search::{DepthSummary, RootSearchExit, SearchOptions, SearchStats, search_root};
use thera::{self, board::Board, move_generator::MoveGenerator};

fn perft_stockfish(fen: &str, depth: u32, preapplied_moves: &[String]) -> Vec<PerftMove> {
    use std::io::Write;
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
enum BisectStep {
    Identical,
    CountDifference {
        thera_count: usize,
        stockfish_count: usize,
        move_: String,
        board: Board,
    },
    /// Move is missing from thera
    MissingMove {
        move_: String,
        board: Board,
    },
    /// Stockfish didn't generate this move
    InvalidMove {
        move_: String,
        board: Board,
    },
}

struct MagicImprovement {
    values: [u64; 64],
    packed_size: usize,
    padded_size: usize,
    magic_type: MagicType,
    max_bits: u32,
    min_bits: u32,
}

struct SearchFinished {
    best_move: Move,
}

enum SearchFailed {
    NoMoves,
}

#[allow(clippy::large_enum_variant)]
enum TaskOutput {
    Perft(PerftStatistics, Duration),
    PerftNostat(u64, Duration),
    BisectStep(BisectStep),
    MagicImprovement(MagicImprovement),
    SearchFinished(SearchFinished),
    SearchFailed(SearchFailed),
    DepthFinished(DepthSummary),
}

type BackgroundTask =
    Box<dyn FnOnce(&AtomicBool, &SyncSender<Option<TaskOutput>>) -> Option<TaskOutput> + Send>;

struct ReplState {
    board: Board,
    undo_stack: Vec<UndoToken>,

    always_print: bool,
    is_uci: Arc<AtomicBool>,

    cancel_flag: Arc<AtomicBool>,
    work_queue: SyncSender<BackgroundTask>,
}

trait ReplCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String>
    where
        Self: std::marker::Sized;
}

struct PerftCommand {
    /// The number of plies to play before counting leaf nodes
    depth: u32,
    /// don't collect statistics in this perft invocation
    nostat: bool,
}

fn missing_subcommand_text(locator: &(impl AsRef<str> + ?Sized)) -> String {
    format!("Missing subcommand in {}", locator.as_ref())
}
fn invalid_subcommand_text(
    subcmd: &(impl AsRef<str> + ?Sized),
    locator: &(impl AsRef<str> + ?Sized),
) -> String {
    format!(
        "Invalid subcommand {} in {}",
        subcmd.as_ref(),
        locator.as_ref()
    )
}
fn missing_argument_text(
    arg: &(impl AsRef<str> + ?Sized),
    locator: &(impl AsRef<str> + ?Sized),
) -> String {
    format!(
        "Missing argument <{}> in {}",
        arg.as_ref(),
        locator.as_ref()
    )
}
fn invalid_argument_text(
    arg: &(impl AsRef<str> + ?Sized),
    error: &(impl AsRef<str> + ?Sized),
    locator: &(impl AsRef<str> + ?Sized),
) -> String {
    format!(
        "Invalid argument <{}> in {}: {}",
        arg.as_ref(),
        locator.as_ref(),
        error.as_ref()
    )
}
fn unknown_argument_text(
    arg: &(impl AsRef<str> + ?Sized),
    locator: &(impl AsRef<str> + ?Sized),
) -> String {
    format!("Unknown argument {} in {}", arg.as_ref(), locator.as_ref(),)
}

fn invalid_value_text(
    arg: &(impl AsRef<str> + ?Sized),
    value: &(impl AsRef<str> + ?Sized),
    locator: &(impl AsRef<str> + ?Sized),
) -> String {
    format!(
        "Invalid value {} for argument <{}> in {}",
        value.as_ref(),
        arg.as_ref(),
        locator.as_ref(),
    )
}

impl ReplCommand for PerftCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            depth: tokens
                .next()
                .ok_or_else(|| missing_argument_text("depth", "perft command"))?
                .parse()
                .map_err(|_| invalid_argument_text("depth", "Not an integer", "perft command"))?,
            nostat: match tokens.next() {
                Some("nostat") => true,
                Some(other) => return Err(unknown_argument_text(other, "perft command")),
                None => false,
            },
        })
    }
}

struct BisectCommand {
    /// The number of plies to play before counting leaf nodes
    depth: u32,
}
impl ReplCommand for BisectCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            depth: tokens
                .next()
                .ok_or_else(|| missing_argument_text("depth", "bisect command"))?
                .parse()
                .map_err(|_| invalid_argument_text("depth", "Not an integer", "bisect command"))?,
        })
    }
}

struct PrintCommand {
    /// Configure the engine to print the board after every command.
    always: Option<bool>,
}

impl ReplCommand for PrintCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            always: match tokens.next() {
                Some("always") => Some(
                    match tokens
                        .next()
                        .ok_or_else(|| missing_argument_text("always", "print command"))?
                    {
                        "true" => true,
                        "false" => false,
                        other => return Err(invalid_value_text(other, "always", "print command")),
                    },
                ),
                Some(other) => return Err(unknown_argument_text(other, "print command")),
                None => None,
            },
        })
    }
}
struct PlayCommand {
    /// The move to play given in long algebraic notation.
    move_: String,
}
impl ReplCommand for PlayCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            move_: tokens
                .next()
                .ok_or_else(|| missing_argument_text("move", "play command"))?
                .to_string(),
        })
    }
}
struct MagicCommand {
    // Which piece to generate magic bitboards for
    type_: MagicType,
}
impl ReplCommand for MagicCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            type_: match tokens
                .next()
                .ok_or_else(|| missing_argument_text("type", "magic command"))?
            {
                "rook" => MagicType::Rook,
                "bishop" => MagicType::Bishop,
                other => return Err(invalid_value_text("type", other, "magic command")),
            },
        })
    }
}
struct GoCommand {
    search_options: SearchOptions,
}
impl ReplCommand for GoCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        let mut search_options = SearchOptions {
            depth: None,
            movetime: None,
            wtime: None,
            btime: None,
            winc: None,
            binc: None,
        };
        while let Some(token) = tokens.next() {
            match token {
                "depth" => {
                    search_options.depth = Some(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("depth", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("depth", "Not an integer", "go command")
                            })?,
                    )
                }
                "movetime" => {
                    search_options.movetime = Some(Duration::from_millis(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("movetime", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("movetime", "Not an integer", "go command")
                            })?,
                    ))
                }
                "wtime" => {
                    search_options.wtime = Some(Duration::from_millis(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("wtime", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("wtime", "Not an integer", "go command")
                            })?,
                    ))
                }
                "btime" => {
                    search_options.btime = Some(Duration::from_millis(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("btime", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("btime", "Not an integer", "go command")
                            })?,
                    ))
                }
                "winc" => {
                    search_options.winc = Some(Duration::from_millis(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("winc", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("winc", "Not an integer", "go command")
                            })?,
                    ))
                }
                "binc" => {
                    search_options.binc = Some(Duration::from_millis(
                        tokens
                            .next()
                            .ok_or_else(|| missing_argument_text("binc", "go command"))?
                            .parse()
                            .map_err(|_| {
                                invalid_argument_text("binc", "Not an integer", "go command")
                            })?,
                    ))
                }

                other => return Err(unknown_argument_text(other, "go command")),
            }
        }

        Ok(Self { search_options })
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
enum MagicType {
    /// Generate rook magic values
    Rook,
    /// Generate bishop magic values
    Bishop,
}

struct MoveSuffixCommand {
    /// Apply a series of moves given in long algebraic notation after loading the position.
    moves: Vec<String>,
}
impl ReplCommand for MoveSuffixCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(Self {
            moves: tokens.map(ToString::to_string).collect_vec(),
        })
    }
}

enum PositionCommand {
    /// Loads the starting position for vanilla chess.
    Startpos { moves: Option<MoveSuffixCommand> },
    /// Loads the position specified in the FEN string.
    Fen {
        /// The FEN string to load.
        fen: Vec<String>,
        moves: Option<MoveSuffixCommand>,
    },
}
impl ReplCommand for PositionCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(
            match tokens
                .next()
                .ok_or_else(|| missing_subcommand_text("position command"))?
            {
                "startpos" => Self::Startpos {
                    moves: Some(MoveSuffixCommand::parse(tokens)?),
                },
                "fen" => Self::Fen {
                    fen: tokens
                        .take_while(|token| *token != "moves") // also takes the "moves" token
                        .map(ToString::to_string)
                        .collect_vec(),
                    moves: Some(MoveSuffixCommand::parse(tokens)?),
                },
                other => return Err(invalid_subcommand_text(other, "position command")),
            },
        )
    }
}
enum TopLevelCommand {
    /// Runs a perft test on the current position. Outputs a breakdown of nodes for each move as
    /// well as the total number of nodes in the same format as stockfish.
    Perft(PerftCommand),
    /// Runs a perft test on the current position. Also feeds the position into stockfish. In case
    /// any moves aren't matching, one of the incorrect ones is played and another bisect is done
    /// on the new position. Reports the FEN string of the respective position as well as the series
    /// of move mismatches.
    Bisect(BisectCommand),
    /// Set up a position on the internal board.
    Position(PositionCommand),
    /// Prints the current board.
    Print(PrintCommand),
    /// Stops the currently running task (if any).
    Stop,
    /// Exits Thera
    Quit,
    /// Undoes the last move that was made.
    Undo,
    /// Plays the given move on the board.
    Play(PlayCommand),
    /// Enables UCI mode.
    Uci,
    /// Resets all internal state to play a new game.
    UciNewGame,
    /// Pings the engine which will respond with "readyok" once it is ready.
    IsReady,
    /// Generates magic bitboards forever
    Magic(MagicCommand),
    /// Start searching for the best move
    Go(GoCommand),
}

impl ReplCommand for TopLevelCommand {
    fn parse<'a>(tokens: &mut impl Iterator<Item = &'a str>) -> Result<Self, String> {
        Ok(
            match tokens.next().ok_or_else(|| "Missing command".to_string())? {
                "perft" => TopLevelCommand::Perft(PerftCommand::parse(tokens)?),
                "bisect" => TopLevelCommand::Bisect(BisectCommand::parse(tokens)?),
                "position" => TopLevelCommand::Position(PositionCommand::parse(tokens)?),
                "print" => TopLevelCommand::Print(PrintCommand::parse(tokens)?),
                "stop" => TopLevelCommand::Stop,
                "quit" | "exit" => TopLevelCommand::Quit,
                "undo" => TopLevelCommand::Undo,
                "play" => TopLevelCommand::Play(PlayCommand::parse(tokens)?),
                "uci" => TopLevelCommand::Uci,
                "ucinewgame" => TopLevelCommand::UciNewGame,
                "isready" => TopLevelCommand::IsReady,
                "magic" => TopLevelCommand::Magic(MagicCommand::parse(tokens)?),
                "go" => TopLevelCommand::Go(GoCommand::parse(tokens)?),
                other => return Err(format!("Invalid command {other}")),
            },
        )
    }
}

fn repl_handle_line(state: &mut ReplState, line: &str) -> Result<bool, String> {
    let command = TopLevelCommand::parse(&mut line.split_whitespace())?;
    match command {
        TopLevelCommand::Perft(subcmd) => {
            repl_handle_perft(state, subcmd);
        }
        TopLevelCommand::Bisect(subcmd) => {
            repl_handle_bisect(state, subcmd);
        }
        TopLevelCommand::Position(subcmd) => {
            repl_handle_position(state, subcmd);
        }
        TopLevelCommand::Print(subcmd) => {
            repl_handle_print(state, subcmd);
        }
        TopLevelCommand::Stop => {
            repl_handle_stop(state);
        }
        TopLevelCommand::Undo => {
            repl_handle_undo(state);
        }
        TopLevelCommand::Play(subcmd) => {
            repl_handle_play(state, subcmd);
        }
        TopLevelCommand::Uci => {
            repl_handle_uci(state);
        }
        TopLevelCommand::UciNewGame => {
            repl_handle_ucinewgame(state);
        }
        TopLevelCommand::IsReady => {
            repl_handle_isready(state);
        }
        TopLevelCommand::Magic(subcmd) => {
            repl_handle_magic(state, subcmd);
        }
        TopLevelCommand::Go(subcmd) => {
            repl_handle_go(state, subcmd);
        }
        TopLevelCommand::Quit => return Ok(true),
    }

    if state.always_print {
        println!("{}", state.board.dump_ansi(None));
    }

    Ok(false)
}

fn apply_move(board: &mut Board, algebraic_move: &str) -> Option<UndoToken> {
    let possible_moves = MoveGenerator::<true>::with_attacks(board).generate_all_moves(board);

    possible_moves
        .into_iter()
        .find(|m2| m2.to_algebraic() == algebraic_move)
        .map(|found_move| board.make_move(found_move))
}

fn repl_handle_perft(state: &mut ReplState, cmd: PerftCommand) {
    let mut board_copy = state.board.clone();
    let spawn_attempt = if cmd.nostat {
        state.work_queue.try_send(Box::new(move |cancel_flag, _| {
            let start = Instant::now();
            perft_nostats(&mut board_copy, cmd.depth, &|| {
                cancel_flag.load(Ordering::Relaxed)
            })
            .map(|node_count| TaskOutput::PerftNostat(node_count, Instant::now() - start))
        }))
    } else {
        state.work_queue.try_send(Box::new(move |cancel_flag, _| {
            let start = Instant::now();
            perft(&mut board_copy, cmd.depth, &|| {
                cancel_flag.load(Ordering::Relaxed)
            })
            .map(|stats| TaskOutput::Perft(stats, Instant::now() - start))
        }))
    };

    match spawn_attempt {
        Ok(()) => {
            println!("Started perft.");
        }
        Err(mpsc::TrySendError::Full(_)) => {
            println!("A task is already running. Not starting perft.");
        }
        Err(mpsc::TrySendError::Disconnected(_)) => {
            println!("The task thread died.");
        }
    }
}

fn repl_handle_position(state: &mut ReplState, cmd: PositionCommand) {
    state.undo_stack.clear();
    match cmd {
        PositionCommand::Startpos { moves } => {
            state.board = Board::starting_position();
            if let Some(MoveSuffixCommand { moves }) = moves {
                for m in moves {
                    if let Some(undo_state) = apply_move(&mut state.board, &m) {
                        state.undo_stack.push(undo_state);
                    } else {
                        println!("The move {m} isn't possible in the current position.");
                        println!("Previous moves are still applied and on the stack");
                        break;
                    }
                }
            }
        }
        PositionCommand::Fen { fen, moves } => {
            let fen = fen.join(" ");
            match Board::from_fen(&fen) {
                Ok(new_board) => {
                    state.board = new_board;
                    if let Some(MoveSuffixCommand { moves }) = moves {
                        for m in moves {
                            if let Some(undo_state) = apply_move(&mut state.board, &m) {
                                state.undo_stack.push(undo_state);
                            } else {
                                println!("The move {m} isn't possible in the current position.");
                                println!("Previous moves are still applied and on the stack");
                                break;
                            }
                        }
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
fn repl_handle_print(state: &mut ReplState, cmd: PrintCommand) {
    if let Some(enable) = cmd.always {
        state.always_print = enable;
    }
    if !state.always_print {
        println!("{}", state.board.dump_ansi(None));
    }
}

fn repl_handle_stop(state: &mut ReplState) {
    state.cancel_flag.store(true, Ordering::SeqCst);
}

fn repl_handle_undo(state: &mut ReplState) {
    if let Some(undo_state) = state.undo_stack.pop() {
        let m = state.board.undo_move(undo_state);
        println!("Undid move {}", m.to_algebraic())
    } else {
        println!("There are no moves on the stack.")
    }
}

fn repl_handle_play(state: &mut ReplState, cmd: PlayCommand) {
    if let Some(undo_state) = apply_move(&mut state.board, &cmd.move_) {
        state.undo_stack.push(undo_state);
    } else {
        println!(
            "The move {} isn't possible in the current position.",
            cmd.move_
        );
    }
}
fn repl_handle_uci(state: &mut ReplState) {
    state.always_print = false;
    state.is_uci.store(true, Ordering::SeqCst);

    println!("id name Thera v{}", env!("CARGO_PKG_VERSION"));
    println!("id author Robotino");
    println!("uciok");
}
fn repl_handle_ucinewgame(state: &mut ReplState) {
    state.board = Board::starting_position();
    state.undo_stack.clear();
}
fn repl_handle_isready(_state: &mut ReplState) {
    println!("readyok");
}
fn repl_handle_magic(state: &mut ReplState, cmd: MagicCommand) {
    let piece = match cmd.type_ {
        MagicType::Rook => Piece::Rook,
        MagicType::Bishop => Piece::Bishop,
    };

    let starting_values = match cmd.type_ {
        MagicType::Rook => &ROOK_MAGIC_VALUES,
        MagicType::Bishop => &BISHOP_MAGIC_VALUES,
    };

    let spawn_attempt = state
        .work_queue
        .try_send(Box::new(move |cancel_flag, result_sender| {
            let mut rng = rand::rng();

            let mut best_magic: [Option<(MagicTableEntry, u64)>; 64] = starting_values
                .iter()
                .cloned()
                .enumerate()
                .map(|(square, magic)| {
                    generate_magic_entry(Bitboard(1 << square), magic, piece).unwrap_or((
                        MagicTableEntry {
                            magic: 0,
                            mask: Bitboard(0),
                            bits_used: 64,
                        },
                        1 << 32,
                    ))
                })
                .map(Option::Some)
                .collect_array()
                .unwrap();

            /*
            let mut best_magic = [Some((
                MagicTableEntry {
                    magic: 0,
                    mask: Bitboard(0),
                    bits_used: 64,
                },
                1 << 32,
            )); 64];
            */

            while !cancel_flag.load(Ordering::Relaxed) {
                let mut has_improvement = false;
                for square in Square::ALL {
                    // magic numbers shouldn't have too many set bits
                    let magic = rng.random::<u64>() & rng.random::<u64>() & rng.random::<u64>();
                    let Some((new_magic, new_entry_size)) =
                        generate_magic_entry(Bitboard::from_square(square), magic, piece)
                    else {
                        continue;
                    };

                    if new_entry_size
                        < best_magic[square as usize]
                            .map(|(_magic, entry)| entry)
                            .unwrap_or(u64::MAX)
                    {
                        best_magic[square as usize] = Some((new_magic, new_entry_size));
                        has_improvement = true;
                    }
                }

                if has_improvement {
                    let packed_table_size = best_magic
                        .iter()
                        .flatten()
                        .map(|(_magic, max_entry)| *max_entry + 1)
                        .sum::<u64>() as usize
                        * size_of::<Bitboard>();
                    let padded_table_size = best_magic
                        .iter()
                        .flatten()
                        .map(|(_magic, max_entry)| *max_entry + 1)
                        .max()
                        .unwrap_or_default() as usize
                        * size_of::<Bitboard>()
                        * 64;

                    result_sender
                        .send(Some(TaskOutput::MagicImprovement(MagicImprovement {
                            packed_size: packed_table_size,
                            padded_size: padded_table_size,
                            min_bits: best_magic
                                .iter()
                                .flatten()
                                .map(|(magic, _max_entries)| magic.bits_used)
                                .min()
                                .unwrap_or_default(),
                            max_bits: best_magic
                                .iter()
                                .flatten()
                                .map(|(magic, _max_entries)| magic.bits_used)
                                .max()
                                .unwrap_or_default(),
                            magic_type: cmd.type_,
                            values: best_magic
                                .iter()
                                .map(|x| x.map(|x| x.0.magic).unwrap_or_default())
                                .collect_array::<64>()
                                .unwrap(),
                        })))
                        .unwrap();
                }
            }

            None
        }));

    match spawn_attempt {
        Ok(()) => {
            println!("Started magic generation.");
        }
        Err(mpsc::TrySendError::Full(_)) => {
            println!("A task is already running. Not starting magic generation.");
        }
        Err(mpsc::TrySendError::Disconnected(_)) => {
            println!("The task thread died.");
        }
    }
}

fn repl_handle_bisect(state: &mut ReplState, cmd: BisectCommand) {
    // clone the board so we don't have to restore it
    let board_clone = state.board.clone();

    let spawn_attempt = state
        .work_queue
        .try_send(Box::new(move |cancel_flag, result_sender| {
            let mut board = board_clone;
            let fen = board.to_fen();
            let mut move_stack = vec![];

            'next_depth: for depth in (1..=cmd.depth).rev() {
                let movegen = MoveGenerator::<true>::with_attacks(&mut board);
                let moves = movegen.generate_all_moves(&board);

                let mut thera_out =
                    perft(&mut board, depth, &|| cancel_flag.load(Ordering::Relaxed))?.divide;
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
                            result_sender
                                .send(Some(TaskOutput::BisectStep(BisectStep::CountDifference {
                                    thera_count: m.nodes,
                                    stockfish_count: equiv.nodes,
                                    move_: m.algebraic_move.clone(),
                                    board: board.clone(),
                                })))
                                .unwrap();

                            let m = moves
                                .into_iter()
                                .find(|actual_move| actual_move.to_algebraic() == m.algebraic_move)
                                .expect("Thera produced a move it doesn't know about");

                            move_stack.push(m.to_algebraic());
                            let _ = board.make_move(m);
                            continue 'next_depth;
                        }
                    } else {
                        return Some(TaskOutput::BisectStep(BisectStep::InvalidMove {
                            move_: m.algebraic_move,
                            board,
                        }));
                    }
                }
                if let Some(m) = stockfish_out.first() {
                    return Some(TaskOutput::BisectStep(BisectStep::MissingMove {
                        move_: m.algebraic_move.clone(),
                        board,
                    }));
                }

                break;
            }

            Some(TaskOutput::BisectStep(BisectStep::Identical))
        }));

    match spawn_attempt {
        Ok(()) => {
            println!("Started bisect.");
        }
        Err(mpsc::TrySendError::Full(_)) => {
            println!("A task is already running. Not starting bisect.");
        }
        Err(mpsc::TrySendError::Disconnected(_)) => {
            println!("The task thread died.");
        }
    }
}
fn repl_handle_go(state: &mut ReplState, cmd: GoCommand) {
    let mut board_clone = state.board.clone();

    let spawn_attempt =
        state.work_queue.try_send(Box::new(
            move |cancel_flag, result_sender| match search_root(
                &mut board_clone,
                cmd.search_options,
                |summary| {
                    result_sender
                        .send(Some(TaskOutput::DepthFinished(summary)))
                        .unwrap();
                },
                || cancel_flag.load(Ordering::Relaxed),
            ) {
                Ok(best_move) => Some(TaskOutput::SearchFinished(SearchFinished { best_move })),
                Err(RootSearchExit::NoMove) => {
                    Some(TaskOutput::SearchFailed(SearchFailed::NoMoves))
                }
            },
        ));

    match spawn_attempt {
        Ok(()) => {
            if !state.is_uci.load(Ordering::Relaxed) {
                println!("Started search.");
            }
        }
        Err(mpsc::TrySendError::Full(_)) => {
            println!("A task is already running. Not starting search.");
        }
        Err(mpsc::TrySendError::Disconnected(_)) => {
            println!("The task thread died.");
        }
    }
}

struct ExternalPrinterWriter<P: ExternalPrinter + Send> {
    printer: P,
    buffer: String,
}

struct StdoutWriter;
impl Write for StdoutWriter {
    fn write_str(&mut self, s: &str) -> std::fmt::Result {
        use std::io::Write;
        stdout()
            .lock()
            .write(s.as_bytes())
            .map_err(|_| std::fmt::Error)
            .map(|_| ())
    }
}

impl<P> ExternalPrinterWriter<P>
where
    P: ExternalPrinter + Send,
{
    pub fn new(printer: P) -> Self {
        Self {
            printer,
            buffer: String::new(),
        }
    }
}

impl<P> Write for ExternalPrinterWriter<P>
where
    P: ExternalPrinter + Send,
{
    fn write_str(&mut self, s: &str) -> std::fmt::Result {
        self.buffer.push_str(s);

        while let Some((line, new_buffer)) = self.buffer.split_once("\n") {
            let line = line.to_string();
            self.buffer = new_buffer.to_string();

            self.printer
                .print(line + "\n")
                .map_err(|_| std::fmt::Error)?
        }
        Ok(())
    }
}

fn output_thread_task(
    results: Receiver<Option<TaskOutput>>,
    mut writer: impl Write,
    is_uci: Arc<AtomicBool>,
) {
    loop {
        let task_output = results.recv();

        let prefix = if is_uci.load(Ordering::SeqCst) {
            "info string "
        } else {
            ""
        };

        fn print_board(writer: &mut impl Write, prefix: &'static str, board: &Board) {
            writeln!(
                writer,
                "{prefix}{}",
                board
                    .dump_ansi(None)
                    .replace("\n", &("\n".to_string() + prefix))
            )
            .unwrap();
        }

        match task_output {
            Ok(Some(TaskOutput::Perft(perft_results, duration))) => {
                for PerftMove {
                    algebraic_move,
                    nodes,
                } in perft_results.divide.iter().sorted()
                {
                    writeln!(writer, "{prefix}{algebraic_move}: {nodes}").unwrap();
                }
                writeln!(writer).unwrap();
                writeln!(
                    writer,
                    "{prefix}Nodes searched: {}",
                    perft_results.node_count
                )
                .unwrap();
                writeln!(
                    writer,
                    "{prefix}Time spent: {}s ({} MN/s)",
                    duration.as_secs_f32(),
                    perft_results.node_count as f32 / duration.as_secs_f32() / 1e6
                )
                .unwrap();
            }
            Ok(Some(TaskOutput::PerftNostat(node_count, duration))) => {
                writeln!(writer, "{prefix}Nodes searched: {}", node_count).unwrap();
                writeln!(
                    writer,
                    "{prefix}Time spent: {}s ({} MN/s)",
                    duration.as_secs_f32(),
                    node_count as f32 / duration.as_secs_f32() / 1e6
                )
                .unwrap();
            }
            Ok(Some(TaskOutput::BisectStep(perft_results))) => match perft_results {
                BisectStep::Identical => {
                    writeln!(writer, "{prefix}All moves are identical").unwrap();
                }
                BisectStep::CountDifference {
                    thera_count,
                    stockfish_count,
                    move_,
                    board,
                } => {
                    print_board(&mut writer, prefix, &board);
                    writeln!(writer, "{prefix}{}", board.to_fen()).unwrap();
                    writeln!(
                        writer,
                        "{prefix}Move {} has {} nodes for Thera, but {} for stockfish. \
                            Recusing into it",
                        move_, thera_count, stockfish_count
                    )
                    .unwrap();
                }
                BisectStep::InvalidMove { move_, board } => {
                    print_board(&mut writer, prefix, &board);
                    writeln!(writer, "{prefix}{}", board.to_fen()).unwrap();
                    writeln!(writer, "{prefix}Move {} is missing from stockfish", move_).unwrap();
                }
                BisectStep::MissingMove { move_, board } => {
                    print_board(&mut writer, prefix, &board);
                    writeln!(writer, "{prefix}{}", board.to_fen()).unwrap();
                    writeln!(writer, "{prefix}Move {} is missing from Thera", move_).unwrap();
                }
            },
            Ok(Some(TaskOutput::MagicImprovement(MagicImprovement {
                packed_size,
                padded_size,
                magic_type,
                min_bits,
                max_bits,
                values,
            }))) => {
                let magic_name = match magic_type {
                    MagicType::Rook => "rook",
                    MagicType::Bishop => "bishop",
                };
                writeln!(writer, "Magic {magic_name} values improved!").unwrap();
                writeln!(writer, "packed: {}kB", packed_size / 1024).unwrap();
                writeln!(writer, "padded: {}kB", padded_size / 1024).unwrap();
                writeln!(writer, "min_bits: {min_bits}, max_bits: {max_bits}").unwrap();
                writeln!(writer, "{values:?}").unwrap();
            }
            Ok(Some(TaskOutput::SearchFinished(SearchFinished { best_move }))) => {
                writeln!(writer, "bestmove {best_move}").unwrap();
            }
            Ok(Some(TaskOutput::SearchFailed(fail_reason))) => {
                writeln!(
                    writer,
                    "Search failed: {}",
                    match fail_reason {
                        SearchFailed::NoMoves => "No legal moves found",
                    }
                )
                .unwrap();
            }
            Ok(Some(TaskOutput::DepthFinished(DepthSummary {
                best_move,
                eval,
                depth,
                time_taken,
                stats: SearchStats { nodes_searched },
            }))) => {
                let time_taken_ms = time_taken.as_millis();
                let nps = nodes_searched as f64 / time_taken.as_secs_f64();

                let eval = eval.to_uci();

                writeln!(
                    writer,
                    "info depth {depth} score {eval} nodes {nodes_searched} nps {nps:.0} time {time_taken_ms} pv {best_move}",
                )
                .unwrap();
            }
            Ok(None) => {
                writeln!(writer, "{prefix}Task was interrupted.").unwrap();
            }
            Err(RecvError) => break,
        }
    }
}

fn main() {
    let (task_sender, task_receiver) = mpsc::sync_channel::<BackgroundTask>(0);
    let (result_sender, result_receiver) = mpsc::sync_channel::<Option<TaskOutput>>(5);

    let cancel_flag = Arc::new(AtomicBool::new(false));
    let cancel_flag2 = cancel_flag.clone();

    let task_thread = std::thread::Builder::new()
        .name("tasks".to_string())
        .spawn(move || {
            while let Ok(task) = task_receiver.recv() {
                cancel_flag2.store(false, Ordering::SeqCst);
                let out = task(&cancel_flag2, &result_sender);
                if result_sender.send(out).is_err() {
                    break;
                }
            }
        })
        .unwrap();

    let mut state = ReplState {
        board: Board::starting_position(),
        undo_stack: Vec::new(),
        always_print: false,
        is_uci: Arc::new(AtomicBool::new(false)),
        cancel_flag,
        work_queue: task_sender,
    };

    let is_uci2 = state.is_uci.clone();

    let output_thread_builder = std::thread::Builder::new().name("output".to_string());

    let output_thread: std::thread::JoinHandle<_>;
    if atty::is(Stream::Stdout) {
        let mut line_editor = DefaultEditor::new().unwrap();

        let printer = ExternalPrinterWriter::new(line_editor.create_external_printer().unwrap());
        output_thread = output_thread_builder
            .spawn(move || output_thread_task(result_receiver, printer, is_uci2))
            .unwrap();
        loop {
            let prompt = if state.is_uci.load(Ordering::SeqCst) {
                ""
            } else {
                "Thera〉"
            };
            match line_editor.readline(prompt) {
                Ok(line) => {
                    if line.trim().is_empty() {
                        continue;
                    }
                    line_editor.add_history_entry(&line).unwrap();
                    match repl_handle_line(&mut state, &line) {
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
                Err(ReadlineError::Interrupted) => {
                    repl_handle_stop(&mut state);
                }
                Err(err) => {
                    println!("{err}");
                }
            }
        }
        drop(line_editor);
    } else {
        output_thread = output_thread_builder
            .spawn(move || output_thread_task(result_receiver, StdoutWriter, is_uci2))
            .unwrap();

        for line in stdin().lock().lines() {
            let line = line.unwrap();
            if line.trim().is_empty() {
                continue;
            }
            match repl_handle_line(&mut state, &line) {
                Ok(quit) => {
                    if quit {
                        break;
                    }
                }
                Err(err) => {
                    println!("info string error: {err}");
                }
            }
        }
    };

    // Explicit drop in case we ever place something below here.
    // This undoes any signal hooks so Ctrl-C works again.

    let ReplState {
        board: _,
        undo_stack: _,
        always_print: _,
        is_uci: _,
        cancel_flag,
        work_queue,
    } = state;

    cancel_flag.store(true, Ordering::Relaxed);
    drop(work_queue); // make sure the task thread exits
    task_thread.join().unwrap();
    output_thread.join().unwrap();
}
