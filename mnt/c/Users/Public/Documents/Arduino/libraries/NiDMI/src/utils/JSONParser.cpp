#include "JSONParser.h"

// Parsing JSON optimisé
int JSONParser::extractInt(const String& src, const char* key, int def) {
    String pat = String("\"") + key + "\":";
    int p = src.indexOf(pat);
    if (p < 0) return def;
    p += pat.length();
    
    while (p < (int)src.length() && (src[p] == ' ')) p++;
    int end = p;
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
    
    int end = src.indexOf('"', p);
    if (end < 0) return def;
    return src.substring(p, end);
}
