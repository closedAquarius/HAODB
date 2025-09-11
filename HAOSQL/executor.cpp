#include "executor.h"

using namespace std;

Scan::Scan(const Table& t) :table(t) {}
vector<Row> Scan::execute() {
	return this->table;
}

Condition::Condition() {}
Condition::Condition(function<bool(const Row&)> f) :func(f) {}
function<bool(const Row&)> Condition::get() const {
	return this->func;
}

Filter::Filter(Operator* c, function<bool(const Row&)> p) : child(c), predicate(p) {}
vector<Row> Filter::execute() {
	vector<Row> input = child->execute();
	vector<Row> output;
	for (auto& row : input) {
		if (predicate(row)) {
			output.push_back(row);
		}
	}

	return output;
}

Project::Project(Operator* c, const vector<string>& cols) : child(c), columns(cols) {}
vector<Row> Project::execute() {
	vector<Row> input = child->execute();
	vector<Row> output;
	for (auto& row : input) {
		Row newRow;
		for (auto& col : columns) {
			newRow[col] = row.at(col);
		}
		output.push_back(newRow);
	}

	return output;
}


Table Students = {
	{{"id", "1"}, {"name", "Alice"  }, {"age", "23"}, {"grade", "A"}},
	{{"id", "2"}, {"name", "Bob"    }, {"age", "19"}, {"grade", "B"}},
	{{"id", "3"}, {"name", "Charlie"}, {"age", "25"}, {"grade", "A"}},
	{{"id", "4"}, {"name", "John"   }, {"age", "18"}, {"grade", "A"}}
};
Table getData(string tableName) {
	// 从 q.arg1 找表数据
	cout << endl << "访问数据库表(文件系统): " << tableName << endl;
	return Students;
}

void split(const string& str, const string& splits, vector<string>& result) {
	if (str == "") return;
	// 末尾加上分隔符，方便截取
	string strs = str + splits;
	size_t pos = strs.find(splits);
	size_t step = splits.size();

	// 若找不到splits则返回
	while (pos != str.npos) {
		string temp = strs.substr(0, pos);
		result.push_back(temp);
		// 去掉已分割的字符串
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}

void getColumns(vector<string>& cols, string s) {
	split(s, ",", cols);
}

Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns) {
	Operator* root = nullptr;
	map<string, Operator*> symbolTables;
	map<string, Condition> condTable;
	Table t;

	for (auto& q : quads) {
		// ========== 基本表扫描 ==========
		if (q.op == "FROM") {
			t = getData(q.arg1);
			symbolTables[q.result] = new Scan(t);
		}

		// ========== 连接 ==========

		// ========== 选择（行选择） ==========
		else if (q.op == ">" || q.op == "=") {
			condTable[q.result] = Condition::simple(q.op, q.arg1, q.arg2);
		}
		else if (q.op == "AND") {
			condTable[q.result] = Condition::And(condTable[q.arg1], condTable[q.arg2]);
		}
		else if (q.op == "OR") {
			condTable[q.result] = Condition::Or(condTable[q.arg1], condTable[q.arg2]);
		}
		else if (q.op == "NOT") {
			condTable[q.result] = Condition::Not(condTable[q.arg1]);
		}
		else if (q.op == "WHERE") {
			Operator* child = symbolTables[q.arg1];
			Condition cond = condTable[q.arg2];
			symbolTables[q.result] = new Filter(child, cond.get());
		}

		// ========== 投影（列选择） ==========
		else if (q.op == "SELECT") {
			getColumns(columns, q.arg1);
			Operator* child = symbolTables[q.arg2];				// 条件筛选过后的表
			root = new Project(child, columns);
		}

		// ========== 排序 ==========

		// ========== 分组聚合 ==========

		// ========== 输出结果 ==========
		else if (q.op == "RESULT") {
			return root;
		}
	}
	return root;
}