#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
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
public:
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

class Insert : public Operator {			   // 插入
private:
	Operator* child;
	BufferPoolManager* bpm;
	string tableName;
public:
	Insert(Operator* c, BufferPoolManager* b, const string& tName);
	vector<Row> execute();
};

class Set :public Operator {
private:
	string cols;
	string change;
public:
	Set(const string& c, const string& ch);
	vector<Row> execute();
};

class Update :public Operator {
private:
	Operator* child;
	Operator* data;
	BufferPoolManager* bpm;
public:
	Update(Operator* c, Operator* d, BufferPoolManager* b);
	vector<Row> execute();
};

class Delete : public Operator {
private:
	Operator* child;
	BufferPoolManager* bpm;
	string tableName;
public:
	Delete(Operator* c, BufferPoolManager* b, const string& tName);
	vector<Row> execute();
};

bool isNumber(const string& str);
void split(const string& str, const string& splits, vector<string>& result);
void getColumns(vector<string>& cols, string s);

// ========== 构建算子树 ==========
Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns, BufferPoolManager* bpm);

class Sort : public Operator {
private:
	Operator* child;
	vector<string> sortColumns;
public:
	Sort(Operator* c, const vector<string>& cols) : child(c), sortColumns(cols) {}
	vector<Row> execute() override {
		vector<Row> input = child->execute();
		std::sort(input.begin(), input.end(), [&](const Row& a, const Row& b) {
			for (auto& col : sortColumns) {
				if (a.at(col) != b.at(col))
					return a.at(col) < b.at(col);
			}
			return false;
			});
		return input;
	}
};

class Aggregate : public Operator {
private:
	Operator* child;
	std::string groupColumn;
	std::string aggColumn;
	std::string aggFunc;  // SUM, COUNT, AVG
public:
	Aggregate(Operator* c, const std::string& gCol, const std::string& aCol, const std::string& func)
		: child(c), groupColumn(gCol), aggColumn(aCol), aggFunc(func) {
	}

	std::vector<Row> execute() override {
		std::vector<Row> input = child->execute();
		std::map<std::string, std::vector<Row>> groups;

		// 按 groupColumn 分组
		for (auto& row : input) {
			std::string key = row.at(groupColumn);
			groups[key].push_back(row);
		}

		std::vector<Row> output;

		// 遍历每组
		for (auto& p : groups) {
			const std::string& key = p.first;
			std::vector<Row>& rows = p.second;

			Row newRow;
			newRow[groupColumn] = key;

			if (aggFunc == "SUM") {
				int sum = 0;
				for (auto& r : rows) sum += std::stoi(r.at(aggColumn));
				newRow[aggColumn] = std::to_string(sum);
			}
			else if (aggFunc == "COUNT") {
				newRow[aggColumn] = std::to_string(rows.size());
			}
			else if (aggFunc == "AVG") {
				int sum = 0;
				for (auto& r : rows) sum += std::stoi(r.at(aggColumn));
				newRow[aggColumn] = std::to_string(sum / rows.size());
			}

			output.push_back(newRow);
		}

		return output;
	}
};