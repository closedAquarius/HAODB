#include "semantic.h"

// ���ߺ������ַ���ת��д
string SemanticAnalyzer::upper(string s) const {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// ���ߺ�����������ʱ����
string SemanticAnalyzer::newTemp() {
    return "T" + to_string(tempId++);
}

// --- ���߷��� ---
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
    if (pos >= toks->size() || toks->at(pos).type != 2) // 2=��ʶ��
        throw SemanticError("Expected identifier", peek().line, peek().column);
    return toks->at(pos++).value;
}

string SemanticAnalyzer::expectConstOrString() {
    if (pos >= toks->size() || (toks->at(pos).type != 3 && toks->at(pos).type != 6))
        throw SemanticError("Expected constant or string", peek().line, peek().column);
    return toks->at(pos++).value;
}

// --- ���������� ---
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
    string fromTemp = newTemp();
    out.push_back({ "FROM", "TABLEDEF", table, fromTemp });

    expectDelim('(');
    string cols = "";
    while (true) {
        string col = expectIdent();
        string type = expectIdent();
        if (!cols.empty()) cols += ",";
        cols += col + ":" + type;
        if (matchDelim(')')) break;
        expectDelim(',');
    }

    string defTemp = newTemp();
    out.push_back({ "CREATE", cols, fromTemp, defTemp });

    out.push_back({ "RESULT", defTemp, "-", "-" });
    return out;
}


// --- SELECT ---
vector<Quadruple> SemanticAnalyzer::handleSelect() {
    vector<Quadruple> out;
    expectKeyword("SELECT");

    // �������� SELECT ������
    vector<string> cols;
    cols.push_back(expectIdent());

    while (matchDelim(',')) {
        cols.push_back(expectIdent());
    }

    // FROM �Ӿ�
    expectKeyword("FROM");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });

    // WHERE �Ӿ�
    string source = fromTemp;
    if (matchKeyword("WHERE")) {
        string cond = parseOr(out);
        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, cond, whereTemp });
        source = whereTemp;
    }

    // SELECT ��Ԫʽ���ϲ�����������һ���ַ�����
    string colList = "";
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i > 0) colList += ",";
        colList += cols[i];
    }
    string selectTemp = newTemp();
    out.push_back({ "SELECT", colList, source, selectTemp });
    source = selectTemp;

    // ORDER BY �Ӿ�
    if (matchKeyword("ORDER")) {
        expectKeyword("BY");
        string orderCol = expectIdent();
        string orderTemp = newTemp();
        out.push_back({ "ORDER", orderCol, source, orderTemp });
        source = orderTemp;
    }

    // RESULT ��Ԫʽ��ֻ��һ��������
    out.push_back({ "RESULT", source, "-", "-" });

    return out;
}



// --- INSERT ---
vector<Quadruple> SemanticAnalyzer::handleInsert() {
    vector<Quadruple> out;
    expectKeyword("INSERT");
    expectKeyword("INTO");
    string table = expectIdent();  // ����

    // �������б�
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

    // ���� VALUES ��Ԫʽ
    string valuesTemp = newTemp();
    out.push_back({ "VALUES", cols, vals, valuesTemp });

    // ���� INSERT ��Ԫʽ
    string insertTemp = newTemp();
    out.push_back({ "INSERT", table, valuesTemp, insertTemp });

    // ���� RESULT ��Ԫʽ
    out.push_back({ "RESULT", insertTemp, "-", "-" });

    return out;
}

// =====================
// parsePrimary���������������������ű���ʽ
// =====================
string SemanticAnalyzer::parsePrimary(vector<Quadruple>& out) {
    if (matchDelim('(')) {
        string temp = parseOr(out);
        expectDelim(')');
        return temp;
    }
    else {
        return expectIdentOrConst();
    }
}

// 解析简单 WHERE 条件，返回条件临时变量名
string SemanticAnalyzer::parseWhereCondition(vector<Quadruple>& out, const string& sourceTemp) {
    if (!matchKeyword("WHERE")) return sourceTemp;

    // 左边列
    string left = expectIdent();
    expectOp("=");
    // 右边常量
    string right = expectConstOrString();

    // 生成条件临时变量
    string condTemp = newTemp();
    out.push_back({ "=", left, right, condTemp });

    // 生成 WHERE 四元式
    string whereTemp = newTemp();
    out.push_back({ "WHERE", sourceTemp, condTemp, whereTemp });

    return whereTemp;
}


vector<Quadruple> SemanticAnalyzer::handleUpdate() {
    vector<Quadruple> out;

    // ===== UPDATE table =====
    expectKeyword("UPDATE");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });

    string source = fromTemp;

    // ===== 暂存 SET 列和表达式 =====
    expectKeyword("SET");
    struct SetItem { string col; string exprStr; };
    vector<SetItem> setList;

    while (true) {
        string col = expectIdent();
        expectOp("=");

        // 暂时只解析右边表达式为字符串，不生成四元式
        string exprStr;
        if (matchOp("+") || matchOp("-") || matchOp("*") || matchOp("/")) {
            string op = toks->at(pos - 1).value;
            string val = expectIdentOrConst();
            exprStr = op + val;   // "+1" 或 "-5"
        }
        else {
            exprStr = expectIdentOrConst(); // 24 或 age
        }

        setList.push_back({ col, exprStr });

        if (!matchDelim(',')) break;
    }

    // ===== WHERE 条件 =====
    if (matchKeyword("WHERE")) {
        string left = expectIdent();
        expectOp("=");
        string right = expectConstOrString();

        string condTemp = newTemp();
        out.push_back({ "=", left, right, condTemp });

        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, condTemp, whereTemp });
        source = whereTemp;
    }

    // ===== 生成 SELECT + SET 四元式 =====
    for (auto& item : setList) {
        string selectTemp = newTemp();
        out.push_back({ "SELECT", item.col, source, selectTemp });

        string setTemp = newTemp();
        out.push_back({ "SET", selectTemp, item.exprStr, setTemp });

        // UPDATE 使用最后一个 SET 的结果作为 source
        source = setTemp;
    }

    // ===== UPDATE =====
    string updateTemp = newTemp();
    out.push_back({ "UPDATE", source, source, updateTemp }); // UPDATE source -> source
    out.push_back({ "RESULT", updateTemp, "-", "-" });

    return out;
}


// 新增辅助函数
string SemanticAnalyzer::expectIdentOrConst() {
    if (pos >= toks->size()) throw SemanticError("Unexpected end of input", 0, 0);
    string val = toks->at(pos).value;
    pos++;
    return val;
}



// --- DELETE ---
vector<Quadruple> SemanticAnalyzer::handleDelete() {
    vector<Quadruple> out;

    // ===== DELETE FROM table =====
    expectKeyword("DELETE");
    expectKeyword("FROM");
    string table = expectIdent();
    string fromTemp = newTemp();
    out.push_back({ "FROM", table, "-", fromTemp });

    string source = fromTemp;

    // ===== WHERE 条件 =====
    if (matchKeyword("WHERE")) {
        // 简单等号条件解析
        string left = expectIdent();
        expectOp("=");
        string right = expectConstOrString();

        string condTemp = newTemp();
        out.push_back({ "=", left, right, condTemp });

        string whereTemp = newTemp();
        out.push_back({ "WHERE", source, condTemp, whereTemp });
        source = whereTemp;
    }

    // ===== DELETE 操作 =====
    string delTemp = newTemp();
    out.push_back({ "DELETE", "-", source, delTemp });
    source = delTemp;

    // ===== RESULT =====
    out.push_back({ "RESULT", source, "-", "-" });

    return out;
}


// --- WHERE ����ʽ�ݹ��½����� ---
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
