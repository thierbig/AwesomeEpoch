#include "../GameExeConfig.h"
#include <cassert>
#include <iostream>

using namespace GameExeConfig;

int main() {
    // Empty / comment-only / blank input yields no value.
    assert(parseValueFromText("") == "");
    assert(parseValueFromText("# only a comment\n") == "");
    assert(parseValueFromText("\n\n   \n# c\n") == "");

    // First non-comment, non-blank line wins.
    assert(parseValueFromText("# c\nWow.exe\n") == "Wow.exe");
    assert(parseValueFromText("A.exe\nB.exe\n") == "A.exe");

    // Whitespace and CRLF are trimmed.
    assert(parseValueFromText("  Wow.exe  \r\n") == "Wow.exe");

    // Surrounding single or double quotes are stripped.
    assert(parseValueFromText("\"C:\\Games\\Wow.exe\"\n") == "C:\\Games\\Wow.exe");
    assert(parseValueFromText("'E:/wow'\n") == "E:/wow");

    // basename() splits on both separators regardless of host OS.
    assert(basename("C:\\Games\\Wow.exe") == "Wow.exe");
    assert(basename("E:/games/wow/Wow.exe") == "Wow.exe");
    assert(basename("Wow.exe") == "Wow.exe");
    assert(basename("") == "");

    // Defaults are ordered base-client first.
    assert(std::string(defaultNames()[0]) == "Wow.exe");
    assert(std::string(defaultNames()[2]) == "Ascension.exe");

    std::cout << "All GameExeConfig tests passed\n";
    return 0;
}
