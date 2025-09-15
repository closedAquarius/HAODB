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
#include "index_manager.h"
#include "catalog_manager.h"
#include <sstream>

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
public:
	string tableName;
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

class Values : public Operator {
private:
	std::vector<std::string> values;
	std::vector<std::string> columns;
public:
	Values(const std::string& v, const std::string& c);
	std::vector<Row> execute() override;
};

class Insert : public Operator {			   // ����
private:
	Operator* child;
	BufferPoolManager* bpm;
	std::string tableName;
public:
	Insert(Operator* c, BufferPoolManager* b, const std::string& tName);
	std::vector<Row> execute() override;
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
	Delete(Operator* c, BufferPoolManager* b);
	vector<Row> execute();
};

bool isNumber(const string& str);
void split(const string& str, const string& splits, vector<string>& result);
void getColumns(vector<string>& cols, string s);

// ========== ���������� ==========
Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns, BufferPoolManager* bpm, CatalogManager* catalog);

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

// ========== DDL������ ==========

// ����������
class CreateTable : public Operator {
private:
	CatalogManager* catalog_;
	string table_name_;
	vector<vector<string>> column_specs_;
	int page_id_;
	vector<string> primary_keys_;
	vector<string> not_null_columns_;
	vector<string> unique_columns_;

public:
	CreateTable(CatalogManager* catalog, const string& table_name,
		const vector<vector<string>>& column_specs, int page_id = 0)
		: catalog_(catalog), table_name_(table_name), column_specs_(column_specs), page_id_(page_id) {
	}

	void setPrimaryKeys(const vector<string>& keys) { primary_keys_ = keys; }
	void setNotNullColumns(const vector<string>& columns) { not_null_columns_ = columns; }
	void setUniqueColumns(const vector<string>& columns) { unique_columns_ = columns; }

	vector<Row> execute() override {
		try {
			// 1. ����������
			bool success = catalog_->CreateTable(DBName, table_name_, column_specs_, page_id_);
			if (!success) {
				throw runtime_error("Failed to create table: " + table_name_);
			}

			/*// 2. ��������Լ��
			if (!primary_keys_.empty()) {
				for (auto& primary_key : primary_keys_)
				{
					success = catalog_->SetPrimaryKey(DBName, table_name_, primary_key, true);
					if (!success) {
						throw runtime_error("Failed to set primary key for table: " + table_name_);
					}
					std::string pk_index_name = table_name_ + primary_key + "_pk";
					indexManager->CreateIndex(table_name_, pk_index_name, intKeys);
						std::cout << "Primary key index created on table "
							<< table_name_ << " (" << pk_index_name << ")\n";
				}
			}*/


			// 3. ���÷ǿ�Լ��
			if (!not_null_columns_.empty()) {
				for (auto& not_null_column : not_null_columns_)
				{
					success = catalog_->SetNullable(DBName, table_name_, not_null_column, true);
					if (!success) {
						throw runtime_error("Failed to set not null constraint for table: " + table_name_);
					}
				}
			}

			// 4. ����ΨһԼ��
			if (!unique_columns_.empty()) {
				for (auto& unique_column : unique_columns_)
				{
					success = catalog_->SetUnique(DBName, table_name_, unique_column, true);
					if (!success) {
						throw runtime_error("Failed to set unique constraint for table: " + table_name_);
					}
				}
			}

			cout << "Table '" << table_name_ << "' created successfully." << endl;
			return {}; // DDL����������������
		}
		catch (const exception& e) {
			throw runtime_error("CreateTable execution failed: " + string(e.what()));
		}
	}
};

// ɾ��������
class DropTable : public Operator {
private:
	CatalogManager* catalog_;
	string table_name_;
	bool if_exists_;

public:
	DropTable(CatalogManager* catalog, const string& table_name, bool if_exists = false)
		: catalog_(catalog), table_name_(table_name), if_exists_(if_exists) {
	}

	vector<Row> execute() override {
		try {
			// ���ָ����IF EXISTS���ȼ����Ƿ����
			if (if_exists_) {
				bool exists = catalog_->TableExists(DBName, table_name_);
				if (!exists) {
					cout << "Table '" << table_name_ << "' does not exist, skipping drop." << endl;
					return {};
				}
			}

			bool success = catalog_->DropTable(DBName, table_name_);
			if (!success) {
				throw runtime_error("Failed to drop table: " + table_name_);
			}

			cout << "Table '" << table_name_ << "' dropped successfully." << endl;
			return {}; // DDL����������������

		}
		catch (const exception& e) {
			throw runtime_error("DropTable execution failed: " + string(e.what()));
		}
	}
};

// ���������
class AddColumn : public Operator {
private:
	CatalogManager* catalog_;
	string table_name_;
	vector<vector<string>> column_specs_;
	vector<string> not_null_columns_;
	vector<string> unique_columns_;

public:
	AddColumn(CatalogManager* catalog, const string& table_name,
		const vector<vector<string>>& column_specs)
		: catalog_(catalog), table_name_(table_name), column_specs_(column_specs) {
	}

	void setNotNullColumns(const vector<string>& columns) { not_null_columns_ = columns; }
	void setUniqueColumns(const vector<string>& columns) { unique_columns_ = columns; }

	vector<Row> execute() override {
		try {
			// 1. �����
			bool success = catalog_->AddColumns(DBName, table_name_, column_specs_);
			if (!success) {
				throw runtime_error("Failed to add columns to table: " + table_name_);
			}

			// 2. ���÷ǿ�Լ��
			if (!not_null_columns_.empty()) {
				for (auto& not_null_column : not_null_columns_)
				{
					success = catalog_->SetNullable(DBName, table_name_, not_null_column, true);
					if (!success) {
						throw runtime_error("Failed to set not null constraint for table: " + table_name_);
					}
				}
			}

			// 3. ����ΨһԼ��
			if (!unique_columns_.empty()) {
				for (auto& unique_column : unique_columns_)
				{
					success = catalog_->SetUnique(DBName, table_name_, unique_column, true);
					if (!success) {
						throw runtime_error("Failed to set unique constraint for table: " + table_name_);
					}
				}
			}

			cout << "Columns added to table '" << table_name_ << "' successfully." << endl;
			return {}; // DDL����������������

		}
		catch (const exception& e) {
			throw runtime_error("AddColumn execution failed: " + string(e.what()));
		}
	}
};

// ɾ��������
class DropColumn : public Operator {
private:
	CatalogManager* catalog_;
	string table_name_;
	vector<string> column_names_;

public:
	DropColumn(CatalogManager* catalog, const string& table_name,
		const vector<string>& column_names)
		: catalog_(catalog), table_name_(table_name), column_names_(column_names) {
	}

	vector<Row> execute() override {
		try {
			bool success = catalog_->DropColumns(DBName, table_name_, column_names_);
			if (!success) {
				throw runtime_error("Failed to drop columns from table: " + table_name_);
			}

			cout << "Columns dropped from table '" << table_name_ << "' successfully." << endl;
			return {}; // DDL����������������

		}
		catch (const exception& e) {
			throw runtime_error("DropColumn execution failed: " + string(e.what()));
		}
	}
};

// ========== DDL�������ݽṹ ==========

// DDL���������е���ʱ״̬
struct DDLBuildState {
	// �ж������
	vector<string> column_names;
	vector<string> column_types;
	vector<string> column_lengths;
	vector<vector<string>> column_specs;

	// Լ�����
	vector<string> primary_keys;
	vector<string> not_null_columns;
	vector<string> unique_columns;

	// ����
	string table_name;

	// ���״̬
	void clear() {
		column_names.clear();
		column_types.clear();
		column_lengths.clear();
		column_specs.clear();
		primary_keys.clear();
		not_null_columns.clear();
		unique_columns.clear();
		table_name.clear();
	}
};

// ========== ���ߺ��� ==========

// �������ŷָ����ַ����б�
vector<string> parseColumnList(const string& str);

// �����й��
vector<vector<string>> buildColumnSpecs(const vector<string>& names,
	const vector<string>& types,
	const vector<string>& lengths);

extern IndexManager* indexManager;

class IndexScan : public Operator {
private:
	BufferPoolManager* bpm;
	string tableName;
	string column;   // �����У�Ŀǰֻ֧�ֵ���������
	int value;       // ��ֵ��ѯֵ��int ���ͣ�

public:
	IndexScan(BufferPoolManager* bpm, const string& tName, const string& col, int val)
		: bpm(bpm), tableName(tName), column(col), value(val) {}

	vector<Row> execute() override;
};