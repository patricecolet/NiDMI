#include "JSONParser.h"

int JSONParser::extractInt(const String& src, const char* key, int def) {
    String pat = String("\"") + key + "\":";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    while (p < (int)src.length() && (src[p] == ' ')) p++;
    
    // Gérer les valeurs entre guillemets : "key":"123" ou "key":"-200"
    bool quoted = (p < (int)src.length() && src[p] == '"');
    if (quoted) p++;
    
    int end = p;
    if (end < (int)src.length() && src[end] == '-') end++;
    while (end < (int)src.length() && isdigit(src[end])) end++;
    
    if (end > p) return src.substring(p, end).toInt();
    return def;
}

bool JSONParser::extractBool(const String& src, const char* key, bool def) {
    String pat = String("\"") + key + "\":";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    while (p < (int)src.length() && (src[p] == ' ')) p++;
    if (src.startsWith("true", p)) return true;
    if (src.startsWith("false", p)) return false;
    return def;
}

String JSONParser::extractStr(const String& src, const char* key, const String& def) {
    String pat = String("\"") + key + "\":\"";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();

    // Parse JSON string with escape support (\", \\, \n, \r, \t).
    String out;
    out.reserve(64);
    bool escaped = false;

    for (int i = p; i < (int)src.length(); i++) {
        char c = src[i];

        if (escaped) {
            switch (c) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: out += c; break;
            }
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            return out;
        }

        out += c;
    }

    return def;
}
