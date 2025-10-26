#include "src/Utils/CommandLineParser/CommandLineParser.hpp"

bool CommandLineParser::bIsParsed = false;
ParsedOptions CommandLineParser::CommandLine;

std::wstring CommandLineParser::ToLowerW(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c){ return std::towlower(c); });
    return s;
}

// split on first occurrence of delim, return pair(left, right_or_empty)
std::pair<std::wstring, std::wstring> CommandLineParser::SplitOnce(const std::wstring &s, wchar_t delim) {
    size_t pos = s.find(delim);
    if (pos == std::wstring::npos) return {s, L""};
    return { s.substr(0, pos), s.substr(pos + 1) };
}

// split by single char delimiter (no empty collapse)
std::vector<std::wstring> CommandLineParser::Split(const std::wstring &s, wchar_t delim) {
    std::vector<std::wstring> out;
    size_t start = 0;
    while (start <= s.size()) {
        size_t pos = s.find(delim, start);
        if (pos == std::wstring::npos) pos = s.size();
        out.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

// Wide <-> UTF-8 helpers ----------------------------------------------------
std::string CommandLineParser::WideToUtf8(const std::wstring &w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &out[0], size, nullptr, nullptr);
    return out;
}
std::wstring CommandLineParser::Utf8ToWide(const std::string &s) {
    if (s.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], size);
    return out;
}

// Percent-decode a percent-encoded string (assume UTF-8 bytes), return wstring
std::wstring CommandLineParser::UrlDecodeToW(const std::wstring &in) {
    // Convert percent-encoded wide input to a narrow byte string (UTF-8 bytes),
    // then decode percent-escapes into bytes, then convert bytes to wide.
    std::string bytes;
    bytes.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        wchar_t wc = in[i];
        if (wc == L'%') {
            if (i + 2 < in.size()) {
                wchar_t h = in[i+1], l = in[i+2];
                auto hexVal = [](wchar_t c)->int {
                    if (c >= L'0' && c <= L'9') return c - L'0';
                    if (c >= L'A' && c <= L'F') return 10 + (c - L'A');
                    if (c >= L'a' && c <= L'f') return 10 + (c - L'a');
                    return -1;
                };
                int hi = hexVal(h), lo = hexVal(l);
                if (hi >= 0 && lo >= 0) {
                    unsigned char val = (unsigned char)((hi << 4) | lo);
                    bytes.push_back((char)val);
                    i += 2;
                    continue;
                }
            }
            // malformed percent, keep literal '%'
            bytes.push_back('%');
        } else if (wc == L'+') {
            // spaces encoded as +
            bytes.push_back(' ');
        } else {
            // copy as UTF-8 bytes for low-ascii; for non-ascii convert single wide -> utf8
            if (wc < 128) {
                bytes.push_back(static_cast<char>(wc));
            } else {
                // convert single wchar to utf8 by using WideToUtf8 on a temporary wstring
                std::wstring tmp(1, wc);
                std::string utf8 = WideToUtf8(tmp);
                bytes.append(utf8);
            }
        }
    }
    return Utf8ToWide(bytes);
}

// Trim helpers (optional)
std::wstring CommandLineParser::TrimW(const std::wstring &s) {
    size_t a = 0;
    while (a < s.size() && std::iswspace(s[a])) ++a;
    size_t b = s.size();
    while (b > a && std::iswspace(s[b-1])) --b;
    return s.substr(a, b - a);
}

// --- parsing functions -----------------------------------------------------

// Parse mapArg: format is "<mapname>[?key=value[?key2=value2[?...]]]"
MapSpec CommandLineParser::ParseMapArg(const std::wstring &mapArg) {
    MapSpec spec;
    if (mapArg.empty()) return spec;
    // split at first '?'
    auto [mapName, rest] = SplitOnce(mapArg, L'?');
    spec.mapName = TrimW(mapName);

    if (!rest.empty()) {
        // rest contains pairs separated by '?'
        auto pairs = Split(rest, L'?');
        for (auto &p : pairs) {
            if (p.empty()) continue;
            auto kv = SplitOnce(p, L'=');
            std::wstring key = TrimW(kv.first);
            std::wstring value = kv.second;
            // optional: URL-decode both key and value
            key = UrlDecodeToW(key);
            value = UrlDecodeToW(value);
            spec.params.emplace(key, value);
        }
    }
    return spec;
}

// Parse whole argv vector (wstrings). supports "-switch=value" and "-switch value"
ParsedOptions CommandLineParser::ParseArgs(const std::vector<std::wstring> &argv) {
    ParsedOptions result;
    // copy all as positional first (we'll also populate switches)
    result.positional = argv;

    // First, parse positional items into switches where appropriate
    // Walk argv: if token starts with '-' or '/', it's a switch.
    for (size_t i = 0; i < argv.size(); ++i) {
        const std::wstring &tok = argv[i];
        if (tok.empty()) continue;
        if ((tok[0] == L'-' || tok[0] == L'/') && tok.size() > 1) {
            // remove leading '-' or '/'
            std::wstring name;
            std::wstring value;
            // check for -name=value
            size_t eqpos = tok.find(L'=');
            if (eqpos != std::wstring::npos) {
                name = tok.substr(1, eqpos - 1);
                value = tok.substr(eqpos + 1);
            } else {
                name = tok.substr(1);
                // value might be next token if next token does not start with '-'
                if (i + 1 < argv.size()) {
                    const std::wstring &next = argv[i + 1];
                    if (!(next.size() > 0 && (next[0] == L'-' || next[0] == L'/'))) {
                        value = next;
                        ++i; // consume the next token
                    }
                }
            }
            if (!name.empty()) {
                result.switches.emplace(ToLowerW(name), value);
            }
        }
        // else it's a positional token (already in positional vector)
    }

    // Special: parse third argument as map spec if present
    // Typically argv[0] = exe, argv[1] = "server", argv[2] = the map arg
    if (argv.size() >= 3) {
        result.level = ParseMapArg(argv[2]);
    }

    return result;
}

ParsedOptions CommandLineParser::ParseCommandLine() {
	if (bIsParsed) {
		return CommandLine;
	}

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::wstring> out;
	if (argv)
	{
		out.reserve(argc);
		for (int i = 0; i < argc; ++i)
			out.emplace_back(argv[i]);
		LocalFree(argv);
	}

	CommandLine = ParseArgs(out);
	bIsParsed = true;

	return CommandLine;
}

