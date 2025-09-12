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
        }
        // ====== SET ======
        else if (q.op == "SET") {
        }
        else if (q.op == "UPDATE") {
        }

        // ====== ɾ�� ======
        else if (q.op == "DELETE") {
        }

        // ====== ������ ======
        else if (q.op == "RESULT") {
            return root;
        }
    }

    return root;
}


Executor::Executor(IndexManager* idxMgr, const std::string& tableName, DiskManager* disk, int bufferSize)
    : indexMgr_(idxMgr), tableName_(tableName), diskManager_(disk) {
    bufferPool_ = std::make_unique<BufferPoolManager>(bufferSize, diskManager_);
}

// ---------------- Select ----------------
std::vector<Row> Executor::select(const std::string& column, int value) {
    std::vector<Row> output;

    auto indexes = indexMgr_->FindIndexesWithColumns(tableName_, { column });
    if (indexes.empty()) {
        // û��������ֱ�ӷ��ؿ�
        return output;
    }

    const std::string indexName = indexes[0].index_name;
    auto rids = indexMgr_->Search(indexName, value);

    for (auto& rid : rids) {
        Page* page = bufferPool_->fetchPage(rid.page_id);
        if (!page) continue;

        std::string record = page->readRecord(rid.slot_id);
        Row row = parseRecord(record);
        output.push_back(row);

        bufferPool_->unpinPage(rid.page_id, false);
    }

    return output;
}

// ---------------- Insert ----------------
bool Executor::insertRow(const Row& newRow, const RID& rid) {
    //assert(indexMgr_ != nullptr);

    for (const auto& col : newRow) {
        auto indexes = indexMgr_->FindIndexesWithColumns(tableName_, { col.first });
        for (const auto& idxInfo : indexes) {
            int key = std::stoi(col.second);
            if (!indexMgr_->InsertEntry(idxInfo.index_name, key, rid)) {
                std::cerr << "[InsertIndex] Failed to insert key " << key
                    << " into index " << idxInfo.index_name << std::endl;
                return false;
            }
        }
    }
    return true;
}

// ---------------- Delete ----------------
bool Executor::deleteRow(const Row& oldRow, const RID& rid) {
    //assert(indexMgr_ != nullptr);

    for (const auto& col : oldRow) {
        auto indexes = indexMgr_->FindIndexesWithColumns(tableName_, { col.first });
        for (const auto& idxInfo : indexes) {
            int key = std::stoi(col.second);
            if (!indexMgr_->DeleteEntry(idxInfo.index_name, key, rid)) {
                std::cerr << "[DeleteIndex] Failed to delete key " << key
                    << " from index " << idxInfo.index_name << std::endl;
                return false;
            }
        }
    }
    return true;
}

// ---------------- Update ----------------
bool Executor::updateRow(const Row& oldRow, const Row& newRow, const RID& rid) {
    //assert(indexMgr_ != nullptr);

    for (const auto& col : newRow) {
        auto indexes = indexMgr_->FindIndexesWithColumns(tableName_, { col.first });
        for (const auto& idxInfo : indexes) {
            int oldKey = std::stoi(oldRow.at(col.first));
            int newKey = std::stoi(newRow.at(col.first));

            if (oldKey != newKey) {
                if (!indexMgr_->DeleteEntry(idxInfo.index_name, oldKey, rid)) {
                    std::cerr << "[UpdateIndex] Failed to delete old key " << oldKey
                        << " from index " << idxInfo.index_name << std::endl;
                    return false;
                }
                if (!indexMgr_->InsertEntry(idxInfo.index_name, newKey, rid)) {
                    std::cerr << "[UpdateIndex] Failed to insert new key " << newKey
                        << " into index " << idxInfo.index_name << std::endl;
                    return false;
                }
            }
        }
    }

    return true;
}

// ---------------- parseRecord ----------------
Row Executor::parseRecord(const std::string& record) {
    Row row;
    // �򵥼���ÿ����¼�ö��ŷָ�����ֵ��ð�ţ��� "id:1,name:Alice,score:95"
    size_t start = 0;
    while (start < record.size()) {
        size_t commaPos = record.find(',', start);
        std::string token = record.substr(start, commaPos - start);
        size_t colonPos = token.find(':');
        if (colonPos != std::string::npos) {
            std::string key = token.substr(0, colonPos);
            std::string value = token.substr(colonPos + 1);
            row[key] = value;
        }
        if (commaPos == std::string::npos) break;
        start = commaPos + 1;
    }
    return row;
}