#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <regex>
#include <iomanip> // Required for std::setw and std::setfill

// Structure to hold styling information for a piece of text
struct TextSpan {
    std::string text;
    std::string fill_color = "#FFFFFF"; // Default: white text
    std::string font_weight = "normal";
    std::string font_style = "normal";
};

// Map ANSI color codes to SVG color equivalents (hex codes)
const std::map<int, std::string> ansi_colors = {
    {30, "#000000"}, {31, "#CD3131"}, {32, "#0DBC79"}, {33, "#E5E510"},
    {34, "#2472C8"}, {35, "#BC3F99"}, {36, "#11A8CD"}, {37, "#E5E5E5"},
    {90, "#666666"}, {91, "#F14C4C"}, {92, "#23D18B"}, {93, "#F5F543"},
    {94, "#3B8EEA"}, {95, "#D670B2"}, {96, "#29B8DB"}, {97, "#FFFFFF"}
};

// Main function to process the text and generate SVG
void generateSVG(const std::string& input_path, const std::string& output_path) {
    std::ifstream input_file(input_path);
    if (!input_file) {
        std::cerr << "Error: Cannot open input file: " << input_path << std::endl;
        return;
    }

    std::vector<std::vector<TextSpan>> lines_of_spans;
    std::string line;

    // Default styles
    std::string current_color = "#FFFFFF";
    std::string current_weight = "normal";
    std::string current_style = "normal";

    // Regex to find all ANSI escape codes, not just color/style ones.
    const std::regex ansi_regex("\x1B\\[([0-9;?]*[a-zA-Z])");

    while (getline(input_file, line)) {
        std::vector<TextSpan> current_line_spans;
        auto last_pos = line.cbegin();
        
        for (auto it = std::sregex_iterator(line.begin(), line.end(), ansi_regex); it != std::sregex_iterator(); ++it) {
            std::smatch match = *it;
            
            // Add text before the match as a span
            if (match.prefix().length() > 0) {
                 current_line_spans.push_back({match.prefix().str(), current_color, current_weight, current_style});
            }

            // Process the ANSI code
            std::string captured_code = match[1].str();
            
            // Only process SGR (color/style) codes, which end in 'm'.
            // Other codes (like cursor movement) are ignored.
            if (!captured_code.empty() && captured_code.back() == 'm') {
                std::string codes_str = captured_code.substr(0, captured_code.length() - 1);
                
                // Split the codes by semicolon
                std::stringstream ss(codes_str);
                std::vector<std::string> codes;
                std::string segment;
                while(getline(ss, segment, ';')) {
                    codes.push_back(segment);
                }

                // Iterate through codes to handle multi-part sequences like 24-bit color
                for (size_t i = 0; i < codes.size(); ++i) {
                    if (codes[i].empty()) {
                        codes[i] = "0"; // An empty code segment implies a reset
                    }
                    try {
                        int code = std::stoi(codes[i]);
                        if (code == 38) { // Extended foreground color
                            // Check for 24-bit RGB: 38;2;R;G;B
                            if (i + 4 < codes.size() && codes[i+1] == "2") {
                                int r = std::stoi(codes[i+2]);
                                int g = std::stoi(codes[i+3]);
                                int b = std::stoi(codes[i+4]);
                                
                                std::stringstream hex_color;
                                hex_color << "#" << std::setw(2) << std::setfill('0') << std::hex << r
                                          << std::setw(2) << std::setfill('0') << std::hex << g
                                          << std::setw(2) << std::setfill('0') << std::hex << b;
                                current_color = hex_color.str();
                                i += 4; // Skip the consumed color codes
                                continue;
                            }
                        }
                        
                        // Handle standard codes
                        switch (code) {
                            case 0: // Reset
                                current_color = "#FFFFFF";
                                current_weight = "normal";
                                current_style = "normal";
                                break;
                            case 1: current_weight = "bold"; break;
                            case 3: current_style = "italic"; break;
                            case 22: current_weight = "normal"; break;
                            case 23: current_style = "normal"; break;
                            default:
                                if (ansi_colors.count(code)) {
                                    current_color = ansi_colors.at(code);
                                }
                                break;
                        }
                    } catch (const std::invalid_argument& e) {
                        // Ignore non-integer codes
                    } catch (const std::out_of_range& e) {
                        // Ignore codes that are too large
                    }
                }
            }
            last_pos = match.suffix().first;
        }

        // Add remaining text on the line
        if (last_pos != line.cend()) {
            current_line_spans.push_back({std::string(last_pos, line.cend()), current_color, current_weight, current_style});
        }
        
        lines_of_spans.push_back(current_line_spans);
    }
    input_file.close();

    // Start writing the SVG file
    std::ofstream output_file(output_path);
    if (!output_file) {
        std::cerr << "Error: Cannot open output file: " << output_path << std::endl;
        return;
    }

    // --- SVG Header ---
    const int font_size = 16;
    const double char_width = font_size * 0.6; // Approximate width for sizing the canvas
    const int line_height = font_size + 4;
    
    // Calculate SVG dimensions
    size_t max_len = 0;
    for(const auto& line_spans : lines_of_spans) {
        size_t current_len = 0;
        for (const auto& span : line_spans) {
            current_len += span.text.length();
        }
        if (current_len > max_len) {
            max_len = current_len;
        }
    }
    
    int svg_width = (max_len * char_width) + 20;
    int svg_height = (lines_of_spans.size() * line_height) + 20;

    output_file << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << svg_width << R"(" height=")" << svg_height << R"(" version="1.1">)" << std::endl;
    output_file << R"(  <rect width="100%" height="100%" fill="#1E1E1E"/>)" << std::endl; // Dark background

    // --- SVG Content ---
    int y = line_height;
    for (const auto& line_spans : lines_of_spans) {
        // Create a new <text> element for each line. This is the key change.
        // xml:space="preserve" is crucial to respect all whitespace.
        output_file << R"(  <text x="10" y=")" << y << R"(" font-family="monospace" font-size=")" << font_size << R"(px" xml:space="preserve">)";

        for (const auto& span : line_spans) {
            // Escape special SVG characters (<, >, &, ", ')
            std::string escaped_text;
            for (char c : span.text) {
                switch (c) {
                    case '&':  escaped_text += "&amp;"; break;
                    case '<':  escaped_text += "&lt;"; break;
                    case '>':  escaped_text += "&gt;"; break;
                    case '\"': escaped_text += "&quot;"; break;
                    case '\'': escaped_text += "&apos;"; break;
                    default:   escaped_text += c; break;
                }
            }

            // Write the tspan *without* x or y coordinates, for styling only.
            output_file << R"(<tspan fill=")" << span.fill_color 
                        << R"(" font-weight=")" << span.font_weight << R"(" font-style=")" << span.font_style 
                        << R"(">)" << escaped_text << R"(</tspan>)";
        }
        
        output_file << "</text>" << std::endl; // Close the <text> element for the line
        y += line_height;
    }

    output_file << "</svg>" << std::endl;

    output_file.close();
    std::cout << "Successfully converted " << input_path << " to " << output_path << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.txt> <output_file.svg>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];

    generateSVG(input_path, output_path);

    return 0;
}

