#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <string>
#include <vector>
#include <stdexcept>
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
    // CREATE TABLE / SELECT / INSERT / UPDATE / DELETE
    vector<Quadruple> handleCreateTable();
    vector<Quadruple> handleSelect();
    vector<Quadruple> handleInsert();
    vector<Quadruple> handleUpdate();
    vector<Quadruple> handleDelete();

    // WHERE 表达式解析
    string parseOr(vector<Quadruple>& out);
    string parseAnd(vector<Quadruple>& out);
    string parsePrimary(vector<Quadruple>& out);
    string parseComparison(vector<Quadruple>& out);
    string parseOperand();

    // 工具
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
#pragma once
