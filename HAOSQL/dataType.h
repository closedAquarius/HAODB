#include <string>
struct Token {
    int type;       // �ֱ���
    std::string lexeme;  // ����
    int line;       // �к�
    int column;     // �к�
};