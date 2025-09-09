#include "semantic.h"
#include <cctype>
#include <algorithm>
#include <sstream>

// 工具函数：字符串转大写
string SemanticAnalyzer::upper(string s) const {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// 工具函数：生成临时变量
string SemanticAnalyzer::newTemp() {
    return "T" + to_string(tempId++);
}

// --- 工具方法 ---
const Token& SemanticAnalyzer::peek(int k) const {
    if (pos + k >= toks->size())
        throw SemanticError("Unexpected end of input", 0, 0);
    return (*toks)[pos + k];
}

bool SemanticAnalyzer::matchKeyword(const char* kw) {
    if (pos < toks->size() && upper(peek().value) == kw) {
        ++pos;
        return true;
    }
    return false;
}

bool SemanticAnalyzer::matchOp(const char* op) {
    if (pos < toks->size() && peek().value == op) {
        ++pos;
        return true;
    }
    return false;
}

bool SemanticAnalyzer::matchDelim(char ch) {
    if (pos < toks->size() && peek().value.size() == 1 && peek().value[0] == ch) {
        ++pos;
        return true;
    }
    return false;
}

bool SemanticAnalyzer::isKeyword(const char* kw) const {
    return pos < toks->size() && upper(peek().value) == kw;
}

void SemanticAnalyzer::expectKeyword(const char* kw) {
    if (!matchKeyword(kw))
        throw SemanticError("Expected keyword " + string(kw), peek().line, peek().column);
}

void SemanticAnalyzer::expectDelim(char ch) {
    if (!matchDelim(ch))
        throw SemanticError(string("Expected delimiter ") + ch, peek().line, peek().column);
}

void SemanticAnalyzer::expectOp(const char* op) {
    if (!matchOp(op))
        throw SemanticError("Expected operator " + string(op), peek().line, peek().column);
}

string SemanticAnalyzer::expectIdent() {
    if (pos >= toks->size() || toks->at(pos).type != 2) // 2=标识符
        throw SemanticError("Expected identifier", peek().line, peek().column);
    return toks->at(pos++).value;
}

string SemanticAnalyzer::expectConstOrString() {
    if (pos >= toks->size() || toks->at(pos).type != 3) // 3=常数
        throw SemanticError("Expected constant/string", peek().line, peek().column);
    return toks->at(pos++).value;
}

// --- 语义分析入口 ---
vector<Quadruple> SemanticAnalyzer::analyze(const vector<Token>& tokens) {
    toks = &tokens;
    pos = 0;
    tempId = 1;

    if (isKeyword("CREATE")) return handleCreateTable();
    if (isKeyword("SELECT")) return handleSelect();
    if (isKeyword("INSERT")) return handleInsert();
    if (isKeyword("UPDATE")) return handleUpdate();
    if (isKeyword("DELETE")) return handleDelete();

    throw SemanticError("Unsupported SQL statement", peek().line, peek().column);
}

// --- CREATE TABLE ---
vector<Quadruple> SemanticAnalyzer::handleCreateTable() {
    vector<Quadruple> out;
    expectKeyword("CREATE");
    expectKeyword("TABLE");
    string table = expectIdent();
    out.push_back({ "CREATE", "TABLE", table, "-" });

    expectDelim('(');
    while (true) {
        string col = expectIdent();
        string type = expectIdent();
        out.push_back({ "COLUMN", col, type, "-" });
        if (matchDelim(')')) break;
        expectDelim(',');
    }

    out.push_back({ "RESULT", "TO", "ALL", "-" });
    return out;
}

// --- SELECT ---
vector<Quadruple> SemanticAnalyzer::handleSelect() {
    vector<Quadruple> out;
    expectKeyword("SELECT");
    string col1 = expectIdent();
    out.push_back({ "SELECT", col1, "-", newTemp() });
    while (matchDelim(',')) {
        string col = expectIdent();
        out.push_back({ "SELECT", col, "-", newTemp() });
    }
    expectKeyword("FROM");
    string table = expectIdent();
    out.push_back({ "FROM", table, "-", newTemp() });

    if (matchKeyword("WHERE")) {
        string cond = parseOr(out);
        out.push_back({ "WHERE", cond, "-", newTemp() });
    }

    out.push_back({ "RESULT", "TO", "ALL", "-" });
    return out;
}

// --- INSERT ---
vector<Quadruple> SemanticAnalyzer::handleInsert() {
    vector<Quadruple> out;
    expectKeyword("INSERT");
    expectKeyword("INTO");
    string table = expectIdent();
    out.push_back({ "INSERT", "INTO", table, "-" });

    expectKeyword("VALUES");
    expectDelim('(');
    while (true) {
        string val = expectConstOrString();
        out.push_back({ "VALUE", val, "-", "-" });
        if (matchDelim(')')) break;
        expectDelim(',');
    }

    out.push_back({ "RESULT", "TO", "ALL", "-" });
    return out;
}

// --- UPDATE ---
vector<Quadruple> SemanticAnalyzer::handleUpdate() {
    vector<Quadruple> out;
    expectKeyword("UPDATE");
    string table = expectIdent();
    out.push_back({ "UPDATE", table, "-", "-" });

    expectKeyword("SET");
    while (true) {
        string col = expectIdent();
        expectOp("=");
        string val = expectConstOrString();
        out.push_back({ "SET", col, val, "-" });
        if (!matchDelim(',')) break;
    }

    if (matchKeyword("WHERE")) {
        string cond = parseOr(out);
        out.push_back({ "WHERE", cond, "-", newTemp() });
    }

    out.push_back({ "RESULT", "TO", "ALL", "-" });
    return out;
}

// --- DELETE ---
vector<Quadruple> SemanticAnalyzer::handleDelete() {
    vector<Quadruple> out;
    expectKeyword("DELETE");
    expectKeyword("FROM");
    string table = expectIdent();
    out.push_back({ "DELETE", "FROM", table, "-" });

    if (matchKeyword("WHERE")) {
        string cond = parseOr(out);
        out.push_back({ "WHERE", cond, "-", newTemp() });
    }

    out.push_back({ "RESULT", "TO", "ALL", "-" });
    return out;
}

// --- WHERE 表达式递归下降解析 ---
string SemanticAnalyzer::parseOr(vector<Quadruple>& out) {
    string left = parseAnd(out);
    while (matchKeyword("OR")) {
        string right = parseAnd(out);
        string t = newTemp();
        out.push_back({ "OR", left, right, t });
        left = t;
    }
    return left;
}

string SemanticAnalyzer::parseAnd(vector<Quadruple>& out) {
    string left = parseComparison(out);
    while (matchKeyword("AND")) {
        string right = parseComparison(out);
        string t = newTemp();
        out.push_back({ "AND", left, right, t });
        left = t;
    }
    return left;
}

string SemanticAnalyzer::parseComparison(vector<Quadruple>& out) {
    string left = parseOperand();
    if (matchOp("=") || matchOp(">") || matchOp("<") || matchOp(">=") || matchOp("<=")) {
        string op = toks->at(pos - 1).value;
        string right = parseOperand();
        string t = newTemp();
        out.push_back({ op, left, right, t });
        return t;
    }
    return left;
}

string SemanticAnalyzer::parseOperand() {
    if (pos < toks->size()) {
        string v = toks->at(pos).value;
        ++pos;
        return v;
    }
    throw SemanticError("Unexpected end in operand", 0, 0);
}
