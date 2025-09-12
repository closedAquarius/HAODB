#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <functional>
#include "dataType.h"
#include "buffer_pool.h"
#include "index_manager.h"

using namespace std;

// ========== ������ ==========
using Row = map<string, string>;
using Table = vector<Row>;

// ========== ���� ==========
class Condition {							// �����ӣ��ҿ���Filter����
private:
	function<bool(const Row&)> func;
public:
	Condition();
	Condition(function<bool(const Row&)> f);
	function<bool(const Row&)> get() const;

	// ����ν��
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

// ========== ���� ==========
class Operator {
public:
	virtual ~Operator() {}
	virtual vector<Row> execute() = 0;
};

class Scan : public Operator {				// ɨ��
private:
	Table table;
	BufferPoolManager* bpm;
	string tableName;
public:
	Scan(BufferPoolManager* bpm, const string& tName);
	vector<Row> execute();
};

class Filter : public Operator {			// ����
public:
	Operator* child;
	function<bool(const Row&)> predicate;
public:
	Filter(Operator* c, function<bool(const Row&)> p);
	vector<Row> execute();
};

class Project : public Operator {			// ͶӰ
private:
	Operator* child;
	vector<string> columns;
public:
	Project(Operator* c, const vector<string>& cols);
	vector<Row> execute();
};

// ========== �������� ==========
void split(const string& str, const string& splits, vector<string>& result);

void getColumns(vector<string>& cols, string s);

// ========== ���������� ==========
Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns);

/**
 * IndexScan: ���������Ĳ�ѯɨ����
 * ���ָ������ B+ ����������ͨ���������ٻ�ȡ RID��
 * ��ͨ�� BufferPool + DiskManager ��ȡ��Ӧҳ�еļ�¼��
 */
class IndexScan {
public:
	/**
	 * ���캯��
	 * @param idxMgr IndexManager ָ��
	 * @param dbFile ���ݿ��ļ�·��
	 * @param tableName ����
	 * @param col ��ѯ��
	 * @param val ��ѯֵ
	 */
	IndexScan(IndexManager* idxMgr,
		const std::string& dbFile,
		const std::string& tableName,
		const std::string& col,
		int val);

	~IndexScan();

	/**
	 * ִ������ɨ��
	 * @return ���������� Row �б�
	 */
	std::vector<Row> execute();

private:
	IndexManager* indexMgr = nullptr;
	std::string databaseFile;
	std::string tableName;
	std::string column;
	int value;

	std::string indexName;
	bool useIndex = false;

	DiskManager* diskManager = nullptr;
	BufferPoolManager* bpm = nullptr;

	/**
	 * ����ҳ��ԭʼ��¼Ϊ Row
	 * �û���Ҫ���ݴ洢��ʽʵ��
	 */
	Row parseRecord(const std::string& record);
};

class Insert : public Operator {
private:
	Table* table;
	Row newRow;
public:
	Insert(Table* t, const Row& r) : table(t), newRow(r) {}
	vector<Row> execute() override {
		table->push_back(newRow);
		return {};  // �����������Ҫ������
	}
};

class Update : public Operator {
private:
	Table* table;
	function<void(Row&)> updater;
	function<bool(const Row&)> predicate;
public:
	Update(Table* t, function<void(Row&)> u, function<bool(const Row&)> p = nullptr)
		: table(t), updater(u), predicate(p) {}

	vector<Row> execute() override {
		for (auto& row : *table) {
			if (!predicate || predicate(row)) {
				updater(row);
			}
		}
		return {};
	}
};

class Delete : public Operator {
private:
	Operator* child;
	Table* table;
public:
	Delete(Operator* c, Table* t) : child(c), table(t) {}
	vector<Row> execute() override {
		vector<Row> input = child->execute();
		for (auto& row : input) {
			table->erase(std::remove(table->begin(), table->end(), row), table->end());
		}
		return {};  // ɾ��������������
	}
};

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
		: child(c), groupColumn(gCol), aggColumn(aCol), aggFunc(func) {}

	std::vector<Row> execute() override {
		std::vector<Row> input = child->execute();
		std::map<std::string, std::vector<Row>> groups;

		// �� groupColumn ����
		for (auto& row : input) {
			std::string key = row.at(groupColumn);
			groups[key].push_back(row);
		}

		std::vector<Row> output;

		// ����ÿ��
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