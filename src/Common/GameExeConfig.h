#pragma once
#include <string>
#include <istream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <array>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace GameExeConfig {

// Built-in fallback names, highest priority first (base client leads).
inline const std::array<const char*, 3>& defaultNames() {
    static const std::array<const char*, 3> names{
        "Wow.exe", "Project-Epoch.exe", "Ascension.exe" };
    return names;
}

inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const std::size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

inline std::string unquote(const std::string& s) {
    if (s.size() >= 2) {
        const char c = s.front();
        if ((c == '"' || c == '\'') && s.back() == c)
            return s.substr(1, s.size() - 2);
    }
    return s;
}

// First non-blank, non-comment line, trimmed and unquoted. "" if none.
inline std::string parseValue(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        return unquote(t);
    }
    return "";
}

inline std::string parseValueFromText(const std::string& text) {
    std::istringstream ss(text);
    return parseValue(ss);
}

// Filename after the last '/' or '\' (both separators, every OS).
inline std::string basename(const std::string& value) {
    const std::size_t pos = value.find_last_of("/\\");
    return pos == std::string::npos ? value : value.substr(pos + 1);
}

#ifdef _WIN32
inline std::filesystem::path moduleDir() {
    char buf[MAX_PATH] = { 0 };
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::filesystem::current_path();
    return std::filesystem::path(buf).parent_path();
}

// Read gameExeLocation.txt next to the executable; "" if missing/empty.
inline std::string readConfiguredValue() {
    const std::filesystem::path cfg = moduleDir() / "gameExeLocation.txt";
    std::ifstream f(cfg);
    if (!f) return "";
    return parseValue(f);
}

inline std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    const int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}
#endif

} // namespace GameExeConfig
