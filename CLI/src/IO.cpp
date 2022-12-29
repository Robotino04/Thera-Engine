#include "CLI/IO.hpp"

const RGB RGB::Black = {0,0,0};
RGB overlay(RGB base, RGB transparent, float opacity){
	return {
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.red) + opacity*static_cast<float>(transparent.red)),
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.green) + opacity*static_cast<float>(transparent.green)),
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.blue) + opacity*static_cast<float>(transparent.blue))
	};
}

std::string setConditionalColor(bool condition, ANSI::ColorLocation loc){
	if (condition) 	return ANSI::set4BitColor(ANSI::Green, loc);
	else 			return ANSI::set4BitColor(ANSI::Red, loc);
}