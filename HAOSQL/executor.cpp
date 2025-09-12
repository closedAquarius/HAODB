#include "executor.h"

using namespace std;

Scan::Scan(BufferPoolManager* bpm, const string& tName) : bpm(bpm), tableName(tName) {}
vector<Row> Scan::execute() {
	// 假设Students表位于 PageId 0
	PageId pageId = 0;
	vector<Row> output;

	// 通过缓冲池获取页
	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed ro fetch page from buffer pool.");
	}

	// 遍历页中的所有槽，解析记录
	for (int i = 0; i < page->header()->slot_count; i++) {
		string record_str = page->readRecord(i);
		Row row;

		// TODO
		// 假设记录是键值对的字符串形式，例如 "id:1,name:Alice,..."
		vector<string> parts;
		split(record_str, ",", parts);
		for (const auto& p : parts) {
			size_t colon_pos = p.find(":");
			if (colon_pos != string::npos) {
				string key = p.substr(0, colon_pos);
				string value = p.substr(colon_pos + 1);
				row[key] = value;
			}
		}

		output.push_back(row);
	}
	
	// 使用完毕后解除 pin
	bpm->unpinPage(pageId, false);

	return output;
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

Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns, BufferPoolManager* bpm) {
	Operator* root = nullptr;
	map<string, Operator*> symbolTables;
	map<string, Condition> condTable;

	for (auto& q : quads) {
		// ========== 基本表扫描 ==========
		if (q.op == "FROM") {
			symbolTables[q.result] = new Scan(bpm, q.arg1);
		}

        // ====== 条件选择 ======
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
        // ====== 条件选择 ======
        else if (q.op == "WHERE") {
            // 动态获取 FROM 节点，而不是写死 "T1"
            Operator* child = symbolTables[q.arg1];  // arg1 是 FROM 的符号
            Condition cond = condTable[q.arg2];      // arg2 是条件符号
            symbolTables[q.result] = new Filter(child, cond.get()); // 生成 Filter 节点
        }

        // ====== 投影 ======
        else if (q.op == "SELECT") {
            getColumns(columns, q.arg1);
            Operator* child = symbolTables[q.arg2];
            root = new Project(child, columns);
        }

        // ====== 排序 ======
        else if (q.op == "ORDERBY") {
            Operator* child = symbolTables[q.arg1];
            vector<string> sortCols;
            getColumns(sortCols, q.arg1);  // q.arg1 为列名列表
            symbolTables[q.result] = new Sort(child, sortCols);
        }

        // ====== 分组聚合 ======
        else if (q.op == "GROUPBY") {
            Operator* child = symbolTables[q.arg1];
            string groupCol = q.arg1;
            string aggCol = q.arg2;
            string aggFunc = q.result; // SUM, COUNT, AVG
            symbolTables[q.result] = new Aggregate(child, groupCol, aggCol, aggFunc);
        }

        // ====== 插入 ======
        else if (q.op == "INSERT") {
            Table* table = &t;   // 假设 t 对应目标表
            Row row = parseRow(q.arg1); // 自己实现 parseRow 把 arg1 转成 Row
            symbolTables[q.result] = new Insert(table, row);
        }
        // ====== SET ======
        else if (q.op == "SET") {
            Operator* setOp = new SetOp(symbolTables[q.arg2]); // q.arg2 = FROM temp
            symbolTables[q.result] = setOp;
        }
        else if (q.op == "UPDATE") {
        }

        // ====== 删除 ======
        else if (q.op == "DELETE") {
            Operator* child = symbolTables[q.arg1]; // 被删除的行
            Table* table = &t;
            symbolTables[q.result] = new Delete(child, table);
        }

        // ====== 输出结果 ======
        else if (q.op == "RESULT") {
            return root;
        }
    }

    return root;
}





IndexScan::IndexScan(BPlusTree* idx, Table* tbl, const string& col, const int& val)
	: index(idx), table(tbl), column(col), value(val) {}

vector<Row> IndexScan::execute() {
	rids = index->search(value);  // 从 B+ 树查找符合的 RID
	vector<Row> output;
	for (auto& rid : rids) {
		//output.push_back(table->getRow(rid));  // 根据 RID 获取行
	}
	return output;
}
