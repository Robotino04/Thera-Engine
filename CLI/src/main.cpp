#include "ANSI/ANSI.hpp"

#include "CLI/Options.hpp"
#include "CLI/IO.hpp"
#include "CLI/playMode.hpp"
#include "CLI/perftMode.hpp"

#include <iostream>

void printHelp(std::string const& argv0){
	std::cout << ANSI::reset() << "Usage: " << argv0 << " [options]\n" << 
R"(Options:
	-h/--help           Print this helping text
	-i                  Print pieces in inverted colors
	-m/--mode [mode]    Run in given mode. Possible values: "play"," perft" Default: "play"
	-d/--depth [n]		Set the depth for perft mode
	--bulk [fen]		Set perft to bulk counting
	--fen [fen]			Set the FEN string for perft and play mode
)";
}


int main(int argc, const char** argv){
	Options options;

	int i = 0;
	while (i+1 < argc){
		std::string arg = argv[++i];
		if (arg == "-h" || arg == "--help"){
			printHelp(argv[0]);
			return 0;
		}
		else if (arg == "-i"){
			options.invertedColors = true;
		}
		else if (arg == "-m" || arg == "--mode"){
			if (i+1 >= argc){
				std::cout << "Missing mode for \"" << arg << "\" option\n";
				return 1;
			}
			std::string mode = argv[++i];
			if (mode == "play"){
				options.mode = Mode::Play;
			}
			else if (mode == "perft"){
				options.mode = Mode::Perft;
			}
			else{
				std::cout << "Invalid mode \"" << mode << "\" for \"" << arg << "\" option\n";
				return 1;
			}
		}
		else if (arg == "-d" || arg == "--depth"){
			if (i+1 >= argc){
				std::cout << "Missing depth string for \"" << arg << "\" option\n";
				return 1;
			}
			options.perftDepth = std::stoi(argv[++i]);
		}
		else if (arg == "--fen"){
			if (i+1 >= argc){
				std::cout << "Missing fen string for \"" << arg << "\" option\n";
				return 1;
			}
			options.fen = argv[++i];
		}
		else if (arg == "--bulk"){
			options.bulkCounting = true;
		}
		else{
			std::cout << "Unknown option \"" << argv[i] << "\"\n";
			return 1;
		}
	}

	switch (options.mode){
		case Mode::Play: return playMode(options);
		case Mode::Perft: return perftMode(options);
		default:
			std::cout << "Unimplemented mode!\n";
			return 1;
	}

	return 0;
}
