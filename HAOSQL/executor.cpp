#include "executor.h"

using namespace std;

Scan::Scan(BufferPoolManager* bpm, const string& tName) : bpm(bpm), tableName(tName) {}
vector<Row> Scan::execute() {
	// ����Students��λ�� PageId 0
	PageId pageId = 0;
	vector<Row> output;

	// ͨ������ػ�ȡҳ
	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed ro fetch page from buffer pool.");
	}

	// ����ҳ�е����вۣ�������¼
	for (int i = 0; i < page->header()->slot_count; i++) {
		string record_str = page->readRecord(i);
		Row row;

		// TODO
		// �����¼�Ǽ�ֵ�Ե��ַ�����ʽ������ "id:1,name:Alice,..."
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
	
	// ʹ����Ϻ��� pin
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
	// ĩβ���Ϸָ����������ȡ
	string strs = str + splits;
	size_t pos = strs.find(splits);
	size_t step = splits.size();

	// ���Ҳ���splits�򷵻�
	while (pos != str.npos) {
		string temp = strs.substr(0, pos);
		result.push_back(temp);
		// ȥ���ѷָ���ַ���
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
		// ========== ������ɨ�� ==========
		if (q.op == "FROM") {
			symbolTables[q.result] = new Scan(bpm, q.arg1);
		}

        // ====== ����ѡ�� ======
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
        // ====== ����ѡ�� ======
        else if (q.op == "WHERE") {
            // ��̬��ȡ FROM �ڵ㣬������д�� "T1"
            Operator* child = symbolTables[q.arg1];  // arg1 �� FROM �ķ���
            Condition cond = condTable[q.arg2];      // arg2 ����������
            symbolTables[q.result] = new Filter(child, cond.get()); // ���� Filter �ڵ�
        }

        // ====== ͶӰ ======
        else if (q.op == "SELECT") {
            getColumns(columns, q.arg1);
            Operator* child = symbolTables[q.arg2];
            root = new Project(child, columns);
        }

        // ====== ���� ======
        else if (q.op == "ORDERBY") {
            Operator* child = symbolTables[q.arg1];
            vector<string> sortCols;
            getColumns(sortCols, q.arg1);  // q.arg1 Ϊ�����б�
            symbolTables[q.result] = new Sort(child, sortCols);
        }

        // ====== ����ۺ� ======
        else if (q.op == "GROUPBY") {
            Operator* child = symbolTables[q.arg1];
            string groupCol = q.arg1;
            string aggCol = q.arg2;
            string aggFunc = q.result; // SUM, COUNT, AVG
            symbolTables[q.result] = new Aggregate(child, groupCol, aggCol, aggFunc);
        }

        // ====== ���� ======
        else if (q.op == "INSERT") {
            Table* table = &t;   // ���� t ��ӦĿ���
            Row row = parseRow(q.arg1); // �Լ�ʵ�� parseRow �� arg1 ת�� Row
            symbolTables[q.result] = new Insert(table, row);
        }
        // ====== SET ======
        else if (q.op == "SET") {
            Operator* setOp = new SetOp(symbolTables[q.arg2]); // q.arg2 = FROM temp
            symbolTables[q.result] = setOp;
        }
        else if (q.op == "UPDATE") {
        }

        // ====== ɾ�� ======
        else if (q.op == "DELETE") {
            Operator* child = symbolTables[q.arg1]; // ��ɾ������
            Table* table = &t;
            symbolTables[q.result] = new Delete(child, table);
        }

        // ====== ������ ======
        else if (q.op == "RESULT") {
            return root;
        }
    }

    return root;
}





IndexScan::IndexScan(BPlusTree* idx, Table* tbl, const string& col, const int& val)
	: index(idx), table(tbl), column(col), value(val) {}

vector<Row> IndexScan::execute() {
	rids = index->search(value);  // �� B+ �����ҷ��ϵ� RID
	vector<Row> output;
	for (auto& rid : rids) {
		//output.push_back(table->getRow(rid));  // ���� RID ��ȡ��
	}
	return output;
}
