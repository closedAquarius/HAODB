#include <iostream>
#include <sstream>
#include <vector>
#include "dataType.h"
#include <algorithm>

using namespace std;



// --- 工具函数 ---
string trim(const string& s) {
    string result = s;
    result.erase(remove_if(result.begin(), result.end(), ::isspace), result.end());
    return result;
}

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

// --- 核心：语义分析器 ---
vector<Quadruple> generateQuadruples(const string& sql_raw) {
    vector<Quadruple> quads;
    string sql = sql_raw;
    string sql_upper = sql_raw;
    transform(sql_upper.begin(), sql_upper.end(), sql_upper.begin(), ::toupper);

    // --------- CREATE TABLE ---------
    if (sql_upper.find("CREATE TABLE") == 0) {
        size_t pos_table = sql_upper.find("TABLE");
        size_t pos_paren = sql.find("(");

        string tableName = sql.substr(pos_table + 5, pos_paren - (pos_table + 5));
        tableName.erase(remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());

        quads.push_back({ "CREATE", "TABLE", tableName, "-" });

        size_t pos_end = sql.find(")", pos_paren);
        string columnsPart = sql.substr(pos_paren + 1, pos_end - pos_paren - 1);

        stringstream ss(columnsPart);
        string colDef;
        while (getline(ss, colDef, ',')) {
            stringstream colStream(colDef);
            string colName, colType;
            colStream >> colName >> colType;
            quads.push_back({ "COLUMN", colName, colType, "-" });
        }
    }

    // --------- SELECT ---------
    else if (sql_upper.find("SELECT") == 0) {
        size_t pos_select = sql_upper.find("SELECT");
        size_t pos_from = sql_upper.find("FROM");
        size_t pos_where = sql_upper.find("WHERE");

        string select_part = sql.substr(pos_select + 6, pos_from - (pos_select + 6));
        string from_part, where_part;
        if (pos_where != string::npos) {
            from_part = sql.substr(pos_from + 4, pos_where - (pos_from + 4));
            where_part = sql.substr(pos_where + 5);
        }
        else {
            from_part = sql.substr(pos_from + 4);
        }

        // SELECT 列
        vector<string> select_fields = split(select_part, ',');
        int tcount = 1;
        for (auto& f : select_fields) {
            string field = trim(f);
            quads.push_back({ "SELECT", field, "-", "T" + to_string(tcount++) });
        }

        // FROM 表
        string table = trim(from_part);
        quads.push_back({ "FROM", table, "-", "T" + to_string(tcount++) });

        // WHERE 条件（简单处理）
        if (!where_part.empty()) {
            string cond = trim(where_part);
            quads.push_back({ "WHERE", cond, "-", "T" + to_string(tcount++) });
        }
    }

    // --------- INSERT ---------
    else if (sql_upper.find("INSERT INTO") == 0) {
        size_t pos_into = sql_upper.find("INTO");
        size_t pos_values = sql_upper.find("VALUES");

        string tableName = sql.substr(pos_into + 4, pos_values - (pos_into + 4));
        tableName.erase(remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());

        quads.push_back({ "INSERT", "INTO", tableName, "-" });

        size_t pos_paren1 = sql.find("(", pos_values);
        size_t pos_paren2 = sql.find(")", pos_paren1);
        string valuesPart = sql.substr(pos_paren1 + 1, pos_paren2 - pos_paren1 - 1);

        vector<string> values = split(valuesPart, ',');
        for (auto& val : values) {
            string v = trim(val);
            quads.push_back({ "VALUE", v, "-", "-" });
        }
    }

    // --------- UPDATE ---------
    else if (sql_upper.find("UPDATE") == 0) {
        size_t pos_update = sql_upper.find("UPDATE");
        size_t pos_set = sql_upper.find("SET");
        size_t pos_where = sql_upper.find("WHERE");

        string tableName = sql.substr(pos_update + 6, pos_set - (pos_update + 6));
        tableName.erase(remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());
        quads.push_back({ "UPDATE", tableName, "-", "-" });

        string setPart = (pos_where != string::npos)
            ? sql.substr(pos_set + 3, pos_where - (pos_set + 3))
            : sql.substr(pos_set + 3);
        vector<string> assigns = split(setPart, ',');
        for (auto& a : assigns) {
            stringstream ss(a);
            string col, eq, val;
            ss >> col >> eq >> val;
            quads.push_back({ "SET", col, val, "-" });
        }

        if (pos_where != string::npos) {
            string cond = sql.substr(pos_where + 5);
            quads.push_back({ "WHERE", trim(cond), "-", "-" });
        }
    }

    // --------- DELETE ---------
    else if (sql_upper.find("DELETE FROM") == 0) {
        size_t pos_from = sql_upper.find("FROM");
        size_t pos_where = sql_upper.find("WHERE");

        string tableName = (pos_where != string::npos)
            ? sql.substr(pos_from + 4, pos_where - (pos_from + 4))
            : sql.substr(pos_from + 4);
        tableName.erase(remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());
        quads.push_back({ "DELETE", "FROM", tableName, "-" });

        if (pos_where != string::npos) {
            string cond = sql.substr(pos_where + 5);
            quads.push_back({ "WHERE", trim(cond), "-", "-" });
        }
    }

    // 结果结束标记
    quads.push_back({ "RESULT", "TO", "ALL", "-" });
    return quads;
}

// --- 测试 ---
int main() {
    vector<string> sqls = {
        "CREATE TABLE students (id INT, name VARCHAR(50), age INT, gpa FLOAT);",
        "SELECT name, age FROM students WHERE age >= 20;",
        "INSERT INTO students VALUES (1, 'Tom', 21, 3.6);",
        "UPDATE students SET age = 22, gpa = 3.8 WHERE id = 1;",
        "DELETE FROM students WHERE age < 18;"
    };

    for (auto& sql : sqls) {
        cout << "\nSQL: " << sql << endl;
        vector<Quadruple> quads = generateQuadruples(sql);
        for (auto& q : quads) {
            cout << "(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.result << ")" << endl;
        }
    }
    return 0;
}
