#include "semantic.h"
#include <algorithm>
#include <stdexcept>

// =====================
// 工具函数
// =====================
string SemanticAnalyzer::upper(string s) const {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

string SemanticAnalyzer::newTemp() {
    return "T" + to_string(tempId++);
}

const Token& SemanticAnalyzer::peek(int k) const {
    if (pos + k >= toks->size())
        throw SemanticError("Unexpected end of input", 0, 0);
    return (*toks)[pos + k];
}

bool SemanticAnalyzer::matchKeyword(const char* kw) {
    if (pos < toks->size() && upper(peek().value) == kw) { ++pos; return true; }
    return false;
}

bool SemanticAnalyzer::matchOp(const char* op) {
    if (pos < toks->size() && peek().value == op) { ++pos; return true; }
    return false;
}

bool SemanticAnalyzer::matchDelim(char ch) {
    if (pos < toks->size() && peek().value.size() == 1 && peek().value[0] == ch) { ++pos; return true; }
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
    if (pos >= toks->size())
        throw runtime_error("Expected identifier, but reached end of input");

    int t = toks->at(pos).type;
    if (t != 2 && t != 1) // 标识符或者关键字都可以
        throw runtime_error("Expected identifier");

    return toks->at(pos++).value;
}

string SemanticAnalyzer::expectConstOrString() {
    if (pos >= toks->size() || (toks->at(pos).type != 3 && toks->at(pos).type != 6))
        throw SemanticError("Expected constant or string", peek().line, peek().column);
    return toks->at(pos++).value;
}

string SemanticAnalyzer::expectIdentOrConst() {
    if (pos >= toks->size())
        throw SemanticError("Unexpected end of input", 0, 0);
    return toks->at(pos++).value;
}

// =====================
// 主入口
// =====================
vector<Quadruple> SemanticAnalyzer::analyze(const vector<Token>& tokens) {
    toks = &tokens;
    pos = 0;
    tempId = 1;

    if (isKeyword("CREATE")) {
        if (peek(1).value == "TABLE") return handleCreateTable();
        else return handleCreateDatabase();
    }
    if (isKeyword("SELECT")) return handleSelect();
    if (isKeyword("INSERT")) return handleInsert();
    if (isKeyword("UPDATE")) return handleUpdate();
    if (isKeyword("DELETE")) return handleDelete();
    if (isKeyword("ALTER")) return handleAlterTable();
    if (isKeyword("DROP")) return handleDropTable();

    throw SemanticError("Unsupported SQL statement", peek().line, peek().column);
}

// =====================
// CREATE TABLE
// =====================
vector<Quadruple> SemanticAnalyzer::handleCreateTable() {
    vector<Quadruple> out;

    expectKeyword("CREATE");
    expectKeyword("TABLE");
    string tableName = expectIdent();
    expectDelim('(');

    vector<string> colNames, colTypes, colLengths;
    vector<string> primaryCols, notNullCols, uniqueCols;

    while (true) {
        string colName = expectIdent();
        string type = upper(expectIdent());
        string length = "-1";
        if (matchDelim('(')) { length = expectConstOrString(); expectDelim(')'); }

        colNames.push_back(colName);
        colTypes.push_back(type);
        colLengths.push_back(length);

        while (true) {
            if (matchKeyword("PRIMARY")) { expectKeyword("KEY"); primaryCols.push_back(colName); }
            else if (matchKeyword("NOT")) { expectKeyword("NULL"); notNullCols.push_back(colName); }
            else if (matchKeyword("UNIQUE")) { uniqueCols.push_back(colName); }
            else break;
        }

        if (matchDelim(',')) continue;
        if (matchDelim(')')) break;
    }

    string t1 = newTemp(), t2 = newTemp(), t3 = newTemp();
    string namesStr = "", typesStr = "", lengthsStr = "";
    for (size_t i = 0; i < colNames.size(); ++i) {
        if (i > 0) { namesStr += ","; typesStr += ","; lengthsStr += ","; }
        namesStr += colNames[i]; typesStr += colTypes[i]; lengthsStr += colLengths[i];
    }

    out.push_back({ "COLUMN_NAME", namesStr, "-", t1 });
    out.push_back({ "COLUMN_TYPE", typesStr, lengthsStr, t2 });
    out.push_back({ "COLUMNS", t1, t2, t3 });

    string t4 = newTemp();
    out.push_back({ "CREATE_TABLE", tableName, t3, t4 });

    if (!primaryCols.empty()) {
        string t5 = newTemp(), primStr = "";
        for (size_t i = 0; i < primaryCols.size(); ++i) { if (i > 0) primStr += ","; primStr += primaryCols[i]; }
        out.push_back({ "PRIMARY", tableName, primStr, t5 });
    }
    if (!notNullCols.empty()) {
        string t6 = newTemp(), nnStr = "";
        for (size_t i = 0; i < notNullCols.size(); ++i) { if (i > 0) nnStr += ","; nnStr += notNullCols[i]; }
        out.push_back({ "NOT_NULL", tableName, nnStr, t6 });
    }
    if (!uniqueCols.empty()) {
        string t7 = newTemp(), uStr = "";
        for (size_t i = 0; i < uniqueCols.size(); ++i) { if (i > 0) uStr += ","; uStr += uniqueCols[i]; }
        out.push_back({ "UNIQUE", tableName, uStr, t7 });
    }

    return out;
}

// =====================
// CREATE DATABASE
// =====================
vector<Quadruple> SemanticAnalyzer::handleCreateDatabase() {
    vector<Quadruple> out;
    expectKeyword("CREATE");
    expectKeyword("DATABASE");
    string dbName = expectIdent();
    string temp = newTemp();
    out.push_back({ "CREATE_DB", dbName, "_", temp });
    out.push_back({ "RESULT", temp, "-", "-" });
    return out;
}

// =====================
// SELECT
// =====================
vector<Quadruple> SemanticAnalyzer::handleSelect() {
    vector<Quadruple> out;
    expectKeyword("SELECT");
    vector<string> cols; cols.push_back(expectIdent());
    while (matchDelim(',')) cols.push_back(expectIdent());

    expectKeyword("FROM");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });
    string source = fromTemp;

    if (matchKeyword("WHERE")) {
        string cond = parseOr(out);
        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, cond, whereTemp });
        source = whereTemp;
    }

    string colList = "";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) colList += ","; colList += cols[i]; }
    string selectTemp = newTemp();
    out.push_back({ "SELECT", colList, source, selectTemp });
    source = selectTemp;

    if (matchKeyword("ORDER")) { expectKeyword("BY"); string orderCol = expectIdent(); string orderTemp = newTemp(); out.push_back({ "ORDER", orderCol, source, orderTemp }); source = orderTemp; }

    out.push_back({ "RESULT", source, "-", "-" });
    return out;
}

// =====================
// INSERT
// =====================
vector<Quadruple> SemanticAnalyzer::handleInsert() {
    vector<Quadruple> out;
    expectKeyword("INSERT"); expectKeyword("INTO");
    string table = expectIdent();

    expectDelim('(');
    string cols = "";
    while (true) {
        string col = expectIdent();
        if (!cols.empty()) cols += ",";
        cols += col;
        if (matchDelim(')')) break;
        expectDelim(',');
    }

    expectKeyword("VALUES");
    expectDelim('(');
    string vals = "";
    while (true) {
        string val = expectConstOrString();
        if (!vals.empty()) vals += ",";
        vals += val;
        if (matchDelim(')')) break;
        expectDelim(',');
    }

    string valuesTemp = newTemp();
    out.push_back({ "VALUES", cols, vals, valuesTemp });

    string insertTemp = newTemp();
    out.push_back({ "INSERT", table, valuesTemp, insertTemp });

    out.push_back({ "RESULT", insertTemp, "-", "-" });
    return out;
}

// =====================
// UPDATE
// =====================
vector<Quadruple> SemanticAnalyzer::handleUpdate() {
    vector<Quadruple> out;
    expectKeyword("UPDATE");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });
    string source = fromTemp;

    expectKeyword("SET");
    struct SetItem { string col; string exprStr; };
    vector<SetItem> setList;
    while (true) {
        string col = expectIdent();
        expectOp("=");
        string exprStr;
        if (matchOp("+") || matchOp("-") || matchOp("*") || matchOp("/")) {
            string op = toks->at(pos - 1).value;
            string val = expectIdentOrConst();
            exprStr = op + val;
        }
        else exprStr = expectIdentOrConst();
        setList.push_back({ col, exprStr });
        if (!matchDelim(',')) break;
    }

    if (matchKeyword("WHERE")) {
        string left = expectIdent(); expectOp("="); string right = expectConstOrString();
        string condTemp = newTemp();
        out.push_back({ "=", left, right, condTemp });
        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, condTemp, whereTemp });
        source = whereTemp;
    }

    for (auto& item : setList) {
        string selectTemp = newTemp();
        out.push_back({ "SELECT", item.col, source, selectTemp });
        string setTemp = newTemp();
        out.push_back({ "SET", selectTemp, item.exprStr, setTemp });
        source = setTemp;
    }

    string updateTemp = newTemp();
    out.push_back({ "UPDATE", source, source, updateTemp });
    out.push_back({ "RESULT", updateTemp, "-", "-" });
    return out;
}

// =====================
// DELETE
// =====================
vector<Quadruple> SemanticAnalyzer::handleDelete() {
    vector<Quadruple> out;
    expectKeyword("DELETE"); expectKeyword("FROM");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });
    string source = fromTemp;

    if (matchKeyword("WHERE")) {
        string left = expectIdent(); expectOp("="); string right = expectConstOrString();
        string condTemp = newTemp();
        out.push_back({ "=", left, right, condTemp });
        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, condTemp, whereTemp });
        source = whereTemp;
    }

    string delTemp = newTemp();
    out.push_back({ "DELETE", "-", source, delTemp });
    source = delTemp;

    out.push_back({ "RESULT", source, "-", "-" });
    return out;
}

// =====================
// ALTER TABLE
// =====================
vector<Quadruple> SemanticAnalyzer::handleAlterTable() {
    vector<Quadruple> out;
    expectKeyword("ALTER");
    expectKeyword("TABLE");
    string table = expectIdent();

    if (matchKeyword("ADD")) {
        expectKeyword("COLUMN");
        string col = expectIdent();
        string type = upper(expectIdent());
        string len = "-1";
        if (matchDelim('(')) {
            len = expectConstOrString();
            expectDelim(')');
        }

        // 列名和类型
        string colNameTemp = newTemp();
        string colTypeTemp = newTemp();
        string colDefTemp = newTemp();
        out.push_back({ "COLUMN_NAME", col, "_", colNameTemp });
        out.push_back({ "COLUMN_TYPE", type, len, colTypeTemp });
        out.push_back({ "COLUMNS", colNameTemp, colTypeTemp, colDefTemp });

        // 添加列
        string addTemp = newTemp();
        out.push_back({ "ADD_COLUMN", table, colDefTemp, addTemp });

        // ---- 检查附加约束 ----
        while (true) {
            if (matchKeyword("NOT")) {
                expectKeyword("NULL");
                string nnTemp = newTemp();
                out.push_back({ "NOT_NULL", table, col, nnTemp });
            }
            else if (matchKeyword("UNIQUE")) {
                string uqTemp = newTemp();
                out.push_back({ "UNIQUE", table, col, uqTemp });
            }
            else if (matchKeyword("DEFAULT")) {
                string val = expectConstOrString();
                string dfTemp = newTemp();
                out.push_back({ "DEFAULT", col, val, dfTemp });
            }
            else break; // 没有更多约束
        }

        out.push_back({ "RESULT", addTemp, "-", "-" });
        return out;
    }
    else if (matchKeyword("DROP")) {
        expectKeyword("COLUMN");
        string col = expectIdent();
        string dropTemp = newTemp();
        out.push_back({ "DROP_COLUMN", table, col, dropTemp });
        out.push_back({ "RESULT", dropTemp, "-", "-" });
        return out;
    }

    throw SemanticError("Unsupported ALTER TABLE operation", peek().line, peek().column);
}


// =====================
// DROP TABLE
// =====================
vector<Quadruple> SemanticAnalyzer::handleDropTable() {
    vector<Quadruple> out;

    expectKeyword("DROP");
    expectKeyword("TABLE");

    bool ifExists = false;
    if (matchKeyword("IF")) {   // IF -> type=1
        expectKeyword("EXISTS"); // EXISTS -> type=1
        ifExists = true;
    }

    string table = expectIdent(); // 支持关键字或标识符作为表名

    string checkTemp = newTemp();
    out.push_back({ "CHECK_TABLE", table, "_", checkTemp });

    string dropTemp = newTemp();
    out.push_back({ "DROP_TABLE", table, "_", dropTemp });

    out.push_back({ "RESULT", dropTemp, "-", "-" });
    return out;
}

// =====================
// WHERE 条件解析
// =====================
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
    if (pos < toks->size()) return toks->at(pos++).value;
    throw SemanticError("Unexpected end in operand", 0, 0);
}

string SemanticAnalyzer::parsePrimary(vector<Quadruple>& out) {
    if (matchDelim('(')) {
        string temp = parseOr(out);
        expectDelim(')');
        return temp;
    }
    else return expectIdentOrConst();
}

string SemanticAnalyzer::parseExpression(vector<Quadruple>& out) {
    string left = parsePrimary(out);
    while (matchOp("+") || matchOp("-") || matchOp("*") || matchOp("/")) {
        string op = toks->at(pos - 1).value;
        string right = parsePrimary(out);
        string temp = newTemp();
        out.push_back({ op, left, right, temp });
        left = temp;
    }
    return left;
}
