use std::fmt::Write;
use std::io::{BufRead, BufReader, stdin, stdout};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::{Receiver, RecvError, SyncSender};
use std::sync::{Arc, mpsc};

use atty::Stream;
use itertools::Itertools;

use clap::{ArgAction, Parser, Subcommand};

use rustyline::error::ReadlineError;
use rustyline::{DefaultEditor, ExternalPrinter};

use thera::board::{FenParseError, MoveUndoState};
use thera::perft::{PerftMove, PerftStatistics, perft};
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

enum TaskOutput {
    Perft(PerftStatistics),
    BisectStep(BisectStep),
}

type BackgroundTask =
    Box<dyn FnOnce(&AtomicBool, &SyncSender<Option<TaskOutput>>) -> Option<TaskOutput> + Send>;

struct ReplState {
    board: Board,
    undo_stack: Vec<MoveUndoState>,

    always_print: bool,

    cancel_flag: Arc<AtomicBool>,
    work_queue: SyncSender<BackgroundTask>,
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
    /// Stops the currently running task (if any).
    Stop,
    /// Exits Thera
    #[command(alias = "exit")]
    Quit,
    /// Undoes the last move that was made.
    Undo,
    /// Plays the given move on the board.
    Play {
        /// The move to play given in long algebraic notation.
        #[arg(name = "move")]
        move_: String,
    },
}

#[derive(Debug, Subcommand)]
#[command(disable_help_flag = true)]
enum PrintSubcommand {
    /// Configure the engine to print the board after every command.
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
        ReplCommands::Stop => {
            repl_handle_stop(state);
        }
        ReplCommands::Undo => {
            repl_handle_undo(state);
        }
        ReplCommands::Play { move_ } => {
            repl_handle_play(state, move_);
        }
        ReplCommands::Quit => return Ok(true),
    }

    if state.always_print {
        println!("{}", state.board.dump_ansi(None));
    }

    Ok(false)
}

fn apply_move(board: &mut Board, algebraic_move: &str) -> Option<MoveUndoState> {
    let mut possible_moves = vec![];
    MoveGenerator::<true>::with_attacks(board).generate_all_moves(board, &mut possible_moves);

    possible_moves
        .into_iter()
        .find(|m2| m2.to_algebraic() == algebraic_move)
        .map(|found_move| board.make_move(found_move))
}

fn repl_handle_perft(state: &mut ReplState, depth: u32) {
    let mut board_copy = state.board;
    let spawn_attempt = state.work_queue.try_send(Box::new(move |cancel_flag, _| {
        perft(&mut board_copy, depth, &|| {
            cancel_flag.load(Ordering::Relaxed)
        })
        .map(TaskOutput::Perft)
    }));

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
            if let Some(MovesSubcommand::Moves { moves }) = moves {
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
                    if let Some(MovesSubcommand::Moves { moves }) = moves {
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
fn repl_handle_print(state: &mut ReplState, cmd: Option<PrintSubcommand>) {
    if let Some(PrintSubcommand::Always { enable }) = cmd {
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
        let m = undo_state.move_();
        state.board.undo_move(undo_state);
        println!("Undid move {}", m.to_algebraic())
    } else {
        println!("There are no moves on the stack.")
    }
}

fn repl_handle_play(state: &mut ReplState, move_: String) {
    if let Some(undo_state) = apply_move(&mut state.board, &move_) {
        state.undo_stack.push(undo_state);
    } else {
        println!("The move {move_} isn't possible in the current position.");
    }
}

fn repl_handle_bisect(state: &mut ReplState, depth: u32) {
    // clone the board so we don't have to restore it
    let board_clone = state.board;

    let spawn_attempt = state
        .work_queue
        .try_send(Box::new(move |cancel_flag, result_sender| {
            let mut board = board_clone;
            let fen = board.to_fen();
            let mut move_stack = vec![];

            'next_depth: for depth in (1..=depth).rev() {
                let movegen = MoveGenerator::<true>::with_attacks(&mut board);
                let mut moves = Vec::new();
                movegen.generate_all_moves(&board, &mut moves);

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
                                    board,
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

fn output_thread_task(results: Receiver<Option<TaskOutput>>, mut writer: impl Write) {
    loop {
        match results.recv() {
            Ok(Some(TaskOutput::Perft(perft_results))) => {
                for PerftMove {
                    algebraic_move,
                    nodes,
                } in perft_results.divide.iter().sorted()
                {
                    writeln!(writer, "{algebraic_move}: {nodes}").unwrap();
                }
                writeln!(writer).unwrap();
                writeln!(writer, "Nodes searched: {}", perft_results.node_count).unwrap();
            }
            Ok(Some(TaskOutput::BisectStep(perft_results))) => match perft_results {
                BisectStep::Identical => {
                    writeln!(writer, "All moves are identical").unwrap();
                }
                BisectStep::CountDifference {
                    thera_count,
                    stockfish_count,
                    move_,
                    board,
                } => {
                    writeln!(writer, "{}", board.dump_ansi(None)).unwrap();
                    writeln!(writer, "{}", board.to_fen()).unwrap();
                    writeln!(
                        writer,
                        "Move {} has {} nodes for Thera, but {} for stockfish. \
                            Recusing into it",
                        move_, thera_count, stockfish_count
                    )
                    .unwrap();
                }
                BisectStep::InvalidMove { move_, board } => {
                    writeln!(writer, "{}", board.dump_ansi(None)).unwrap();
                    writeln!(writer, "{}", board.to_fen()).unwrap();
                    writeln!(writer, "Move {} is missing from stockfish", move_).unwrap();
                }
                BisectStep::MissingMove { move_, board } => {
                    writeln!(writer, "{}", board.dump_ansi(None)).unwrap();
                    writeln!(writer, "{}", board.to_fen()).unwrap();
                    writeln!(writer, "Move {} is missing from Thera", move_).unwrap();
                }
            },
            Ok(None) => {
                writeln!(writer, "Task was interrupted.").unwrap();
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
        cancel_flag,
        work_queue: task_sender,
    };

    let output_thread_builder = std::thread::Builder::new().name("output".to_string());

    let output_thread: std::thread::JoinHandle<_>;
    if atty::is(Stream::Stdout) {
        let mut line_editor = DefaultEditor::new().unwrap();

        let printer = ExternalPrinterWriter::new(line_editor.create_external_printer().unwrap());
        output_thread = output_thread_builder
            .spawn(move || output_thread_task(result_receiver, printer))
            .unwrap();
        loop {
            match line_editor.readline("Thera〉") {
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
            .spawn(move || output_thread_task(result_receiver, StdoutWriter))
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
        cancel_flag,
        work_queue,
    } = state;

    cancel_flag.store(true, Ordering::Relaxed);
    drop(work_queue); // make sure the task thread exits
    task_thread.join().unwrap();
    output_thread.join().unwrap();
}
