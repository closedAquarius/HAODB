#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "dataType.h"
#include "buffer_pool.h"

using namespace std;

// ========== 表数据 ==========
using Row = map<string, string>;
using Table = vector<Row>;

// ========== 条件 ==========
class Condition {							// 非算子，挂靠在Filter算子
private:
	function<bool(const Row&)> func;
public:
	Condition();
	Condition(function<bool(const Row&)> f);
	function<bool(const Row&)> get() const;

	// 基础谓词
	static Condition simple(string op, string col, string val) {
		return Condition([=](const Row& row) {
			string v = row.at(col);
			if (op == ">")  return stoi(v) > stoi(val);
			if (op == ">=")  return stoi(v) >= stoi(val);
			if (op == "<")  return stoi(v) < stoi(val);
			if (op == "<=")  return stoi(v) <= stoi(val);
			if (op == "=")  return v == val;
			if (op == "!=")  return v != val;
			throw runtime_error("unsupported operator:" + op);
			});
	}

	// AND
	static Condition And(const Condition& c1, const Condition& c2) {
		return Condition([=](const Row& r) { return c1.get()(r) && c2.get()(r); });
	}

	// OR
	static Condition Or(const Condition& c1, const Condition& c2) {
		return Condition([=](const Row& r) { return c1.get()(r) || c2.get()(r); });
	}

	// NOT
	static Condition Not(const Condition& c) {
		return Condition([=](const Row& r) { return !c.get()(r); });
	}
};

// ========== 算子 ==========
class Operator {
public:
	virtual ~Operator() {}
	virtual vector<Row> execute() = 0;
};

class Scan : public Operator {				// 扫描
private:
	Table table;
	BufferPoolManager* bpm;
	string tableName;
public:
	Scan(BufferPoolManager* bpm, const string& tName);
	vector<Row> execute();
};

class Filter : public Operator {			// 过滤
private:
	Operator* child;
	function<bool(const Row&)> predicate;
public:
	Filter(Operator* c, function<bool(const Row&)> p);
	vector<Row> execute();
};

class Project : public Operator {			// 投影
private:
	Operator* child;
	vector<string> columns;
public:
	Project(Operator* c, const vector<string>& cols);
	vector<Row> execute();
};

// ========== 访问数据 ==========
void split(const string& str, const string& splits, vector<string>& result);

void getColumns(vector<string>& cols, string s);

// ========== 构建算子树 ==========
Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns, BufferPoolManager* bpm);