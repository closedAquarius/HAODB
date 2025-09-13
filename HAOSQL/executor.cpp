#include "executor.h"

using namespace std;

Scan::Scan(BufferPoolManager* bpm, const string& tName) : bpm(bpm), tableName(tName) {}
vector<Row> Scan::execute() {
	// TODO: ��Ԫ�����л��tableName��ҳ��ַ
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
		if (row.empty()) continue;
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
		if(row.empty()) continue;
		Row newRow;
		for (auto& col : columns) {
			newRow[col] = row.at(col);
		}
		output.push_back(newRow);
	}

	return output;
}

Insert::Insert(Operator* c, BufferPoolManager* b, const string& tName) : child(c), bpm(b), tableName(tName) {}
vector<Row> Insert::execute() {
	vector<Row> input = child->execute();
	// TODO: ��Ԫ�����л��tableName��ҳ��ַ
	// ����Students��λ�� PageId 0
	PageId pageId = 0;

	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for insert.");
	}

	// ��ÿһ�����ݲ��뵽ҳ��
	for (auto& row : input) {
		string record_str = "";
		bool first = true;
		for (const auto& pair : row) {
			if (!first) record_str += ",";
			record_str += pair.first + ":" + pair.second;
			first = false;
		}
		page->insertRecord(record_str.c_str(), record_str.length());
	}

	bpm->unpinPage(pageId, true); // ���Ϊ��ҳ
	return {}; // INSERT����������
}

Set::Set(const string& c, const string& ch) : cols(c), change(ch) {}
vector<Row> Set::execute() {
	vector<Row> output;
	vector<string> keys, values;
	Row updateData;
	split(cols, ",", keys);
	split(change, ",", values);

	if (!keys.empty()) {
		updateData[keys[0]] = values[0];
		output.push_back(updateData);
	}

	return output;
}

Update::Update(Operator* c, Operator* d, BufferPoolManager* b) :child(c), data(d), bpm(b) {}
vector<Row> Update::execute() {
	// ��ȡ�ӽڵ㣨ͨ���� Filter�����ص���
	vector<Row> rowsToUpdate = child->execute();
	vector<Row> updateData = data->execute();
	vector<Row> output;

	Row updateValues = updateData.front();
	// TODO
	// ������ Scan �� Filter �׶��Ѿ��������ַ�����磺PageId �� SlotId���洢�� Row ������
	// ʵ��ϵͳ�У�Row �������� `rid`��Record ID����������λ��¼
	// �򻯣�ֱ�Ӵ� child ��� Row��Ȼ�������Զ�λ������

	for (auto& row : rowsToUpdate) {
		// α���룺�� Row ��ȡ�������ַ (pageId, slotId)
		// PageId pageId = row.getPageId();
		// SlotId slotId = row.getSlotId();

		// ����ͨ��ĳ�ֻ��ƿ��Ի�ȡ��ҳ�ţ��������Ǽ���Ϊ 0
		PageId pageId = 0; // ����һ���򻯣�

		// ��������ȡҳ��
		Page* page = bpm->fetchPage(pageId);
		if (!page) {
			throw runtime_error("Failed to fetch page for update.");
		}

		// �޸����е�ֵ
		for (const auto& kv : updateValues) {
			row[kv.first] = kv.second;
		}

		// ���޸ĺ�������±���Ϊ�ַ���
		string updatedRecordStr = "";

		for (auto& [k, v] : row) {
			updatedRecordStr += (k + ":" + v + ",");
		}

		// ���¼�¼д��ҳ��Ĳ���
		page->getSlot(0)->length = 0;
		page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());

		// ��ҳ����Ϊ�࣬�Ա����д�ش���
		bpm->unpinPage(pageId, true); // true ��ʾ��ҳ����ҳ

		// �ֶ�д��
		bpm->flushPage(pageId);

		output.push_back(row);
	}

	return output;
}

Delete::Delete(Operator* c, BufferPoolManager* b, const string& tName) : child(c), bpm(b), tableName(tName) {}
vector<Row> Delete::execute() {
	// child ������ִ�к󣬷�����Ҫɾ�����С�
	vector<Row> inputRows = child->execute();
	if (inputRows.empty()) {
		cout << "No rows found to delete.\n";
		return {};
	}

	// TODO: �ҵ� tableName ��Ӧ��ҳ�������Ϊ PageId 0
	PageId pageId = 0;
	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for delete.");
	}

	// ����������Ҫɾ������
	for (auto& row : inputRows) {
		// �ҵ�������ҳ�е�λ�ã��������߼�ɾ��
		// ͬ����������õ�Ч�ı������ҷ�ʽ��
		string key_col = "id";
		string key_val = row.at(key_col);

		int targetSlot = -1;
		for (int i = 0; i < page->header()->slot_count; ++i) {
			string record_str = page->readRecord(i);
			if (record_str.find(key_col + ":" + key_val) != string::npos) {
				targetSlot = i;
				break;
			}
		}

		if (targetSlot != -1) {
			page->deleteRecord(targetSlot);
			cout << "Deleted row with " << key_col << " = " << key_val << endl;
		}
		else {
			cout << "Row with " << key_col << " = " << key_val << " not found." << endl;
		}
	}

	bpm->unpinPage(pageId, true); // ���Ϊ��ҳ
	return {}; // DELETE ����������
}


bool isNumber(const string& str) {
	stringstream ss(str);
	double num;
	return (ss >> num) && ss.eof();
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
			// arg1 �� Value ���ӵķ��ţ�arg2 �Ǳ���
			Operator* child = symbolTables[q.arg1];
			root = new Insert(child, bpm, q.arg2);
		}
		// ====== SET ======
		else if (q.op == "SET") {
			// ���� SET ��Ԫʽ�ĸ�ʽΪ (SET, ����, ֵ, ����)
			symbolTables[q.result] = new Set(q.arg1, q.arg2);
		}
		// ====== ���� ======
		else if (q.op == "UPDATE") {
			Operator* child = symbolTables[q.arg1];
			Operator* data = symbolTables[q.arg2];
			root = new Update(child, data, bpm);
		}

		// ====== ɾ�� ======
		else if (q.op == "DELETE") {
			// arg1 �� FROM �� WHERE ���ӵķ��ţ����ڶ�λҪɾ������
			// q.result �Ǳ���
			Operator* child = symbolTables[q.arg1]; // ��ȡ FROM/WHERE ����
			root = new Delete(child, bpm, q.result);
		}

		// ====== ������ ======
		else if (q.op == "RESULT") {
			return root;
		}
	}

	return root;
}
