#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum ColorTarget {
    Foreground = 38,
    Background = 48,
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum TerminalColor {
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
    Gray = 90,
    BrightRed = 91,
    BrightGreen = 92,
    BrightYellow = 93,
    BrightBlue = 94,
    BrightMagenta = 95,
    BrightCyan = 96,
    BrightWhite = 97,

    Default = 39,
}

pub struct Ansi;
impl Ansi {
    pub fn color24(r: u8, g: u8, b: u8, target: ColorTarget) -> String {
        format!("\x1B[{};2;{r};{g};{b}m", target as u8)
    }
    pub fn color4(color: TerminalColor, target: ColorTarget) -> String {
        format!(
            "\x1B[{}m",
            color as u8
                + match target {
                    ColorTarget::Foreground => 0,
                    ColorTarget::Background => 10,
                }
        )
    }

    pub fn fg_color24(r: u8, g: u8, b: u8) -> String {
        Self::color24(r, g, b, ColorTarget::Foreground)
    }
    pub fn bg_color24(r: u8, g: u8, b: u8) -> String {
        Self::color24(r, g, b, ColorTarget::Background)
    }

    pub fn fg_color4(color: TerminalColor) -> String {
        Self::color4(color, ColorTarget::Foreground)
    }
    pub fn bg_color4(color: TerminalColor) -> String {
        Self::color4(color, ColorTarget::Background)
    }

    pub fn reset() -> &'static str {
        "\x1B[0m"
    }
}
