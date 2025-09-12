#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <sstream>
#include "dataType.h"
using namespace std;

struct SemanticError : public runtime_error {
    int line, col;
    SemanticError(const string& msg, int l, int c)
        : runtime_error(msg), line(l), col(c) {}
};

class SemanticAnalyzer {
public:
    vector<Quadruple> analyze(const vector<Token>& tokens);

private:
    // CRUD ����
    vector<Quadruple> handleCreateTable();
    vector<Quadruple> handleSelect();
    vector<Quadruple> handleInsert();
    vector<Quadruple> handleUpdate();
    vector<Quadruple> handleDelete();

    // WHERE ����ʽ����
    string parseOr(vector<Quadruple>& out);
    string parseAnd(vector<Quadruple>& out);
    string parseComparison(vector<Quadruple>& out);
    string parseOperand();
    string parsePrimary(vector<Quadruple>& out);
    string parseExpression(vector<Quadruple>& out); // 声明 parseExpression
    string expectIdentOrConst(); // 声明 expectIdentOrConst

    string parseWhereCondition(vector<Quadruple>& out, const string& sourceTemp);

    // ����
    const Token& peek(int k = 0) const;
    bool matchKeyword(const char* kw);
    bool matchOp(const char* op);
    bool matchDelim(char ch);
    bool isKeyword(const char* kw) const;
    void expectKeyword(const char* kw);
    void expectDelim(char ch);
    void expectOp(const char* op);
    string expectIdent();
    string expectConstOrString();

    string upper(string s) const;
    string newTemp();

private:
    const vector<Token>* toks = nullptr;
    size_t pos = 0;
    int tempId = 1;
};

#endif
