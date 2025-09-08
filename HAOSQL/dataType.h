#include <string>
struct Token {
    int type;       // 种别码
    std::string lexeme;  // 单词
    int line;       // 行号
    int column;     // 列号
};