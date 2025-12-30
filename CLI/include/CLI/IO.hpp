#pragma once

#include <cstdint>
#include <string>

#include "ANSI/ANSI.hpp"

struct RGB {
    uint8_t red = 0, green = 0, blue = 0;

    bool operator==(RGB const& other) const {
        return this->red == other.red && this->green == other.green && this->blue == other.blue;
    }

    bool operator!=(RGB const& other) const {
        return !(*this == other);
    }

    static const RGB Black;
};

RGB overlay(RGB base, RGB transparent, float opacity);

std::string setConditionalColor(bool condition, ANSI::ColorLocation loc);
