# ANSI to SVG Converter

This project contains two C++ programs that convert text files with ANSI escape codes to SVG images, preserving colors and text styles.

![test1](https://github.com/user-attachments/assets/afd4902f-0a2e-4d08-af40-40e3baddf17a)

## Programs

### `24bit.cpp`
- Supports **24-bit RGB color codes** (true color)
- Allows for millions of colors using `38;2;R;G;B` ANSI sequences
- Also supports standard 8-bit/16-color ANSI codes

### `8bit.cpp`
- Supports **8-bit color codes** (16-color ANSI palette)
- Also supports basic text styles like bold and italic

## Usage

Both programs are used the same way:

```bash
./ansi_to_svg <input_file.txt> <output_file.svg>
```

## Features

- Converts ANSI-colored text to SVG format
- Preserves text styles (bold, italic)
- Uses monospace font for proper terminal-like display
- Dark background (#1E1E1E) for better visibility of ANSI colors
- Proper escaping of special XML characters

## Build Instructions

To compile either program:

```bash
g++ -std=c++11 -o ansi_to_svg 24bit.cpp
# or
g++ -std=c++11 -o ansi_to_svg 8bit.cpp
```

## Example

Given a text file with ANSI codes like:

```bash
echo -e "\e[31mRed Text\e[0m and \e[1;32mbold green\e[0m" > sample.txt
./ansi_to_svg sample.txt output.svg
```

This will create an SVG file showing red and green colored text.

