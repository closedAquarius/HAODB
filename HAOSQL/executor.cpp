#include "executor.h"

using namespace std;

// ȫ�ֱ���
WALDataRecord WAL_DATA_RECORD;
void SET_BEFORE_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len)
{
	WAL_DATA_RECORD.before_page_id = pid;
	WAL_DATA_RECORD.before_slot_id = sid;
	WAL_DATA_RECORD.before_length = len;
}
void SET_AFTER_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len)
{
	WAL_DATA_RECORD.after_page_id = pid;
	WAL_DATA_RECORD.after_slot_id = sid;
	WAL_DATA_RECORD.after_length = len;
}
void SET_SQL_QUAS(string sql, vector<Quadruple> quas)
{
	WAL_DATA_RECORD.sql = sql;
	WAL_DATA_RECORD.quas = quas;
}
std::string LOG_PATH;
void SET_LOG_PATH(std::string path)
{
	LOG_PATH = path;
}

Scan::Scan(BufferPoolManager* bpm, const string& tName, PageId i) :bpm(bpm), tableName(tName), pageOffset(i) {}
vector<Row> Scan::execute() {
	vector<Row> output;

	// ͨ������ػ�ȡҳ
	Page* page = bpm->fetchPage(pageOffset);
	if (!page) {
		throw runtime_error("Failed ro fetch page from buffer pool.");
	}

	// ����ҳ�е����вۣ�������¼
	for (int i = 0; i < page->header()->slot_count; i++) {
		string record_str = page->readRecord(i);
		Row row;

		// ��¼�Ǽ�ֵ�Ե��ַ�����ʽ������ "id:1,name:Alice,..."
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
	bpm->unpinPage(pageOffset, false);

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

// Values ����ʵ��
Values::Values(const std::string& v, const std::string& c) {
	// ����ֵ�б����� "(1,"test",19,"B")"
	// ȥ�����ţ��������ŷָ�
	string trimmed_v = v.substr(1, v.length() - 2);
	split(trimmed_v, ",", this->values);

	// ���������б����� "(id,name,age,grade)"
	string trimmed_c = c.substr(1, c.length() - 2);
	split(trimmed_c, ",", this->columns);

	if (this->values.size() != this->columns.size()) {
		throw std::runtime_error("Values and columns count mismatch.");
	}
}
vector<Row> Values::execute() {
	Row newRow;
	for (size_t i = 0; i < values.size(); ++i) {
		// ȥ��ֵ�����ţ�������ڵĻ�
		string val = values[i];
		if (val.front() == '"' && val.back() == '"') {
			val = val.substr(1, val.length() - 2);
		}
		newRow[columns[i]] = val;
	}
	return { newRow };
}

// Insert ����ʵ��
Insert::Insert(Operator* c, BufferPoolManager* b, const string& tName, PageId i) : child(c), bpm(b), tableName(tName), pageOffset(i) {}
vector<Row> Insert::execute() {
	// ���ӽڵ㣨VALUES ���ӣ���ȡҪ�����������
	vector<Row> input = child->execute();

	PageId pageId = pageOffset;

	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for insert.");
	}

	// ��ȡ�ñ������������Ϣ
	// IndexManager* indexManager; // ����ȫ�ֿ���
	std::cout << "initIndexManager=" << endl;
	std::cout << "���catalog����:" << std::endl;
	std::cout << "catalogָ��: " << indexManager->catalog_ << std::endl;
	std::cout << "thisָ��: " << static_cast<void*>(indexManager->catalog_) << std::endl;

	// ���Է��ʳ�Ա����
	try {
		// ���������Ч����Ӧ����������
		std::cout << "����״̬���ͨ��" << std::endl;
	}
	catch (...) {
		std::cout << "����״̬���ʧ�� - �����������" << std::endl;
	}
	vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable(tableName);

	// ��ÿһ�����ݲ��뵽ҳ��
	for (auto& row : input) {
		string record_str = "";
		bool first = true;
		for (const auto& pair : row) {
			if (!first) record_str += ",";
			record_str += pair.first + ":" + pair.second;
			first = false;
		}

		// ����ҳ����� RID
		int slotId = page->insertRecord(record_str.c_str(), record_str.length());
		RID rid{ pageId, slotId };

		// ������������
		for (auto& idx : tableIndexes) {
			std::cout << "===================================" << endl;
			std::cout << idx.column_names[0] << endl;
			int keyInt = 0;

			if (idx.column_names.size() == 1) {
				// ����������ֱ��ת int
				const string& col = idx.column_names[0];
				keyInt = std::stoi(row.at(col));
			}
			else {
				// ����������ƴ���ַ����� hash �� int
				string combinedKey;
				bool firstKey = true;
				for (auto& col : idx.column_names) {
					if (!firstKey) combinedKey += "|";  // �ָ���
					combinedKey += row.at(col);
					combinedKey += row.at(col);
					firstKey = false;
				}
				std::hash<std::string> hasher;
				keyInt = static_cast<int>(hasher(combinedKey));
			}
		}

		// ��ɲ�����������ʱ
		GLOBAL_TIMER.stop();

		SET_AFTER_WAL_RECORD(pageId, slotId, page->getSlot(slotId)->length);
		// ������־
		enhanced_executor->InsertRecord(WAL_DATA_RECORD.after_page_id, WAL_DATA_RECORD.after_slot_id, WAL_DATA_RECORD.after_length,
			WAL_DATA_RECORD.sql, WAL_DATA_RECORD.quas, USER_NAME, true, GLOBAL_TIMER.elapsed_ms(), "");

		// enhanced_executor->PrintAllWAL();
	}

	bpm->unpinPage(pageId, true); // ���Ϊ��ҳ
	bpm->flushPage(pageId);       // �ֶ�д��

	return {}; // INSERT ����������
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

bool tryParseInt(const std::string& str, int& out) {
	try {
		size_t pos;
		long val = std::stol(str, &pos, 10);  // ���� long �ӣ�����Խ��
		if (pos != str.size()) return false;  // �зǷ��ַ�
		if (val < std::numeric_limits<int>::min() || val > std::numeric_limits<int>::max())
			return false;  // ���� int ��Χ
		out = static_cast<int>(val);
		return true;
	}
	catch (...) {
		return false;  // ת��ʧ��
	}
}

Update::Update(Operator* c, Operator* d, BufferPoolManager* b, PageId i) :child(c), data(d), bpm(b), pageOffset(i) {}
vector<Row> Update::execute() {
	// ��ȡ�ӽڵ㣨ͨ���� Filter�����ص���
	vector<Row> rowsToUpdate = child->execute();
	vector<Row> updateData = data->execute();
	vector<Row> output;

	Row updateValues = updateData.front();

	for (auto& row : rowsToUpdate) {

		PageId pageId = pageOffset;

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

		// �ҵ�������ҳ�е�λ�ã��������߼�ɾ��
		// ͬ����������ñ������ҷ�ʽ��
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
			uint16_t before_length = page->getSlot(targetSlot)->length;
			page->deleteRecord(targetSlot);
			std::cout << "Deleted row with " << key_col << " = " << key_val << endl;

			// ��¼�ϼ�¼λ��
			SET_BEFORE_WAL_RECORD(pageId, targetSlot, before_length);
		}
		else {
			std::cout << "Row with " << key_col << " = " << key_val << " not found." << endl;
		}
		vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable("students"); // TODO: �滻�ɵ�ǰ����
		for (auto& idx : tableIndexes) {
			int id = pageId;
			RID rid{ id, targetSlot };
			int i;
			if (tryParseInt(key_val, i)) {
				indexManager->DeleteEntry(idx.table_name, idx.column_names, i, rid);
			}
		}

		// ���¼�¼д��ҳ��Ĳ���
		page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());
		string newKeyVal = row.at(key_col);
		int newKey = stoi(newKeyVal);
		for (auto& idx : tableIndexes) {
			int id = pageId;
			RID rid{ id, 0 }; // TODO: ���� slot / ҳ�� ���� RID
			indexManager->InsertEntry(idx.table_name, idx.column_names, newKey, rid);
		}
		int newSlot =  page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());

		// ��¼�¼�¼λ��
		SET_AFTER_WAL_RECORD(pageId, newSlot, page->getSlot(newSlot)->length);

		// ��ҳ����Ϊ�࣬�Ա����д�ش���
		bpm->unpinPage(pageId, true); // true ��ʾ��ҳ����ҳ

		// �ֶ�д��
		bpm->flushPage(pageId);

		// ��ɲ�����������ʱ
		GLOBAL_TIMER.stop();

		// ������־
		enhanced_executor->UpdateRecord(WAL_DATA_RECORD.before_page_id, WAL_DATA_RECORD.before_slot_id, WAL_DATA_RECORD.before_length,
			WAL_DATA_RECORD.after_page_id, WAL_DATA_RECORD.after_slot_id, WAL_DATA_RECORD.after_length,
			WAL_DATA_RECORD.sql, WAL_DATA_RECORD.quas, USER_NAME, true, GLOBAL_TIMER.elapsed_ms(), "");

		// enhanced_executor->PrintAllWAL();

		output.push_back(row);
	}

	return output;
}

Delete::Delete(Operator* c, BufferPoolManager* b, PageId i) : child(c), bpm(b), pageOffset(i) {}
vector<Row> Delete::execute() {
	// child ������ִ�к󣬷�����Ҫɾ������
	vector<Row> inputRows = child->execute();
	if (inputRows.empty()) {
		std::cout << "No rows found to delete.\n";
		return {};
	}

	PageId pageId = pageOffset;
	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for delete.");
	}

	// IndexManager* indexManager;
	// ��ȡ�ñ������������Ϣ
	vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable(tableName);

	// ����������Ҫɾ������
	for (auto& row : inputRows) {
		string key_col = "id"; // ���������� id
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
			// ���� RID
			RID rid{ pageId, static_cast<uint16_t>(targetSlot) };

			// ɾ�������������
			for (auto& idx : tableIndexes) {
				int keyInt = 0;

				if (idx.column_names.size() == 1) {
					// ��������
					const string& col = idx.column_names[0];
					keyInt = std::stoi(row.at(col));
				}
				else {
					// �������� �� ��ϳ��ַ����� hash
					string combinedKey;
					bool first = true;
					for (auto& col : idx.column_names) {
						if (!first) combinedKey += "|";
						combinedKey += row.at(col);
						first = false;
					}
					std::hash<std::string> hasher;
					keyInt = static_cast<int>(hasher(combinedKey));
				}

				// ���� IndexManager ɾ��������Ŀ
				indexManager->DeleteEntry(tableName, idx.column_names, keyInt, rid);
			}

			// ɾ�������¼
			page->deleteRecord(targetSlot);
			std::cout << "Deleted row with " << key_col << " = " << key_val << endl;

			// ��ɲ�����������ʱ
			GLOBAL_TIMER.stop();

			// ��¼�ϼ�¼λ��
			SET_BEFORE_WAL_RECORD(pageId, targetSlot, page->getSlot(targetSlot)->length);

			// ������־
			enhanced_executor->DeleteRecord(WAL_DATA_RECORD.before_page_id, WAL_DATA_RECORD.before_slot_id, WAL_DATA_RECORD.before_length,
				WAL_DATA_RECORD.sql, WAL_DATA_RECORD.quas, USER_NAME, true, GLOBAL_TIMER.elapsed_ms(), "");

			// enhanced_executor->PrintAllWAL();
		}
		else {
			std::cout << "Row with " << key_col << " = " << key_val << " not found." << endl;
		}
	}

	bpm->unpinPage(pageId, true); // ���Ϊ��ҳ
	bpm->flushPage(pageId);       // �ֶ�д��

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

// ========== DDL���ߺ��� ==========

vector<string> parseColumnList(const string& str) {
	vector<string> result;
	if (str.empty()) return result;

	size_t start = 0;
	size_t pos = 0;

	while (pos != string::npos) {
		pos = str.find(',', start);
		string token = str.substr(start, pos - start);

		// ȥ���ո�
		size_t first = token.find_first_not_of(" \t");
		size_t last = token.find_last_not_of(" \t");
		if (first != string::npos) {
			result.push_back(token.substr(first, last - first + 1));
		}

		if (pos != string::npos) {
			start = pos + 1;
		}
	}

	return result;
}

vector<vector<string>> buildColumnSpecs(const vector<string>& names,
	const vector<string>& types,
	const vector<string>& lengths) {
	vector<vector<string>> specs;

	if (names.size() != types.size() || names.size() != lengths.size()) {
		throw runtime_error("Column names, types, and lengths must have the same size");
	}

	for (size_t i = 0; i < names.size(); ++i) {
		vector<string> spec = { names[i], types[i], lengths[i] };
		specs.push_back(spec);
	}

	return specs;
}

Operator* buildPlan(const vector<Quadruple>& quads, vector<string>& columns, BufferPoolManager* bpm, CatalogManager* catalog) {
	Operator* root = nullptr;
	map<string, Operator*> symbolTables;
	map<string, Condition> condTable;
	PageId pageOffset;
	// DDL����״̬
	DDLBuildState ddl_state;
	IndexedCondition ic;

	for (auto& q : quads) {
		// ========== ������ɨ�� ==========
		// (FROM, databaseName , - , T(scan))
		if (q.op == "FROM") {
			// ��Ԫ�����л��tableName��ҳ��ַ
			pageOffset = catalog->GetTableInfo(DBName, q.arg1).data_file_offset / 4096;
			symbolTables[q.result] = new Scan(bpm, q.arg1, pageOffset);
		}

		// ====== ����ѡ�� ======
		// (OP, column , value , T(condition))
		else if (q.op == ">" || q.op == "=") {
			condTable[q.result] = Condition::simple(q.op, q.arg1, q.arg2);
			ic.col = q.arg1;
			ic.val = q.arg2;
			ic.op = q.op;
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
		// (WHERE, T(scan) , T(condition) , T(filter))
		else if (q.op == "WHERE") {
			// ��̬��ȡ FROM �ڵ㣬������д�� "T1"
			Operator* child = symbolTables[q.arg1];  // arg1 �� FROM �ķ���
			Condition cond = condTable[q.arg2];      // arg2 ����������
			symbolTables[q.result] = new Filter(child, cond.get()); // ���� Filter �ڵ�
			

			// ��ȡ������Ϣ��Ŀǰֻ֧�� = �ҵ���������
			string colName = ic.col;
			string colValue = ic.val;
			string op = ic.op;

			// �� FROM �ڵ����ñ���
			Scan* scanOp = dynamic_cast<Scan*>(child);
			if (!scanOp) {
				throw runtime_error("WHERE's child is not a Scan operator.");
			}
			string tableName = scanOp->tableName;

			extern IndexManager* indexManager;
			// ���Ҹñ������
			vector<IndexInfo> indexes = indexManager->FindIndexesByTable(tableName);

			bool useIndex = false;
			for (auto& idx : indexes) {
				if (idx.column_names.size() == 1 && idx.column_names[0] == colName && op == "=") {
					// ��������
					int keyVal = stoi(colValue);
					symbolTables[q.result] = new IndexScan(bpm, tableName, colName, keyVal);
					useIndex = true;
					break;
				}
			}

			if (!useIndex) {
				// ���˵� Filter+Scan
				symbolTables[q.result] = new Filter(child, cond.get());
			}
		}

		// ====== ͶӰ ======
		// (SELECT , columns , T(filter) , T(Project))
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

		// ====== VALUES ======
		// (VALUES , values , columns , T(values))
		else if (q.op == "VALUES") {
			// arg1 ��ֵ�б��ַ�����arg2 �������б��ַ���
			// ���� VALUES ���ӣ�������������ű�
			symbolTables[q.result] = new Values(q.arg1, q.arg2);
		}

		// ====== ���� ======
		// (INSERT , T(scan) , T(values) , T(r))
		else if (q.op == "INSERT") {
			// arg1 �� FROM �ķ��ţ���������arg2 �� VALUES �ķ���
			Operator* fromChild = symbolTables[q.arg1];
			Operator* valuesChild = symbolTables[q.arg2];

			// �� FROM ��������ȡ����
			Scan* scanOp = dynamic_cast<Scan*>(fromChild);
			if (scanOp == nullptr) {
				throw runtime_error("INSERT's FROM child is not a Scan operator.");
			}
			string tableName = scanOp->tableName;

			root = new Insert(valuesChild, bpm, tableName, pageOffset);
		}

		// ====== SET ======
		// (SET , column , value , T(set))
		else if (q.op == "SET") {
			// ���� SET ��Ԫʽ�ĸ�ʽΪ (SET, ����, ֵ, ����)
			symbolTables[q.result] = new Set(q.arg1, q.arg2);
		}

		// ====== ���� ======
		// (UPDATE , T(scan) , T(set) , T(r))
		else if (q.op == "UPDATE") {
			Operator* child = symbolTables[q.arg1];
			Operator* data = symbolTables[q.arg2];
			root = new Update(child, data, bpm, pageOffset);
		}

		// ====== ɾ�� ======
		// (DELETE , T(scan) , - , T(r))
		else if (q.op == "DELETE") {
			// arg1 �� FROM �� WHERE ���ӵķ��ţ����ڶ�λҪɾ������
			Operator* child = symbolTables[q.arg2]; // ��ȡ FROM/WHERE ����
			root = new Delete(child, bpm, pageOffset);
		}

		// ========== DDL���� ==========

		// ====== ��������� ======
		else if (q.op == "COLUMN_NAME") {
			// (COLUMN_NAME, id,name,age,phone, -, T1)
			ddl_state.column_names = parseColumnList(q.arg1);
		}
		else if (q.op == "COLUMN_TYPE") {
			// (COLUMN_TYPE, VARCHAR,VARCHAR,INT,VARCHAR, 10,10,-1,15, T2)
			ddl_state.column_types = parseColumnList(q.arg1);
			ddl_state.column_lengths = parseColumnList(q.arg2);

			// ����-1�������INT����Ĭ�ϳ��ȣ�
			for (auto& length : ddl_state.column_lengths) {
				if (length == "-1") {
					length = "4"; // INTĬ��4�ֽ�
				}
			}
		}
		else if (q.op == "COLUMNS") {
			// (COLUMNS, T1, T2, T3) - �ϲ�����Ϣ
			try {
				ddl_state.column_specs = buildColumnSpecs(ddl_state.column_names,
					ddl_state.column_types,
					ddl_state.column_lengths);
			}
			catch (const exception& e) {
				throw runtime_error("Failed to build column specifications: " + string(e.what()));
			}
		}
		else if (q.op == "CREATE_TABLE") {
			// (CREATE_TABLE, student, T3, T4)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for CREATE_TABLE operation");
			}

			ddl_state.table_name = q.arg1;

			// ������ҳid
			int pageId = catalog->GetDatabaseInfo(DBName).table_count;

			CreateTable* create_op = new CreateTable(catalog, ddl_state.table_name, ddl_state.column_specs, pageId, bpm->getDM());
			symbolTables[q.result] = create_op;
			root = create_op;
		}
		else if (q.op == "PRIMARY") {
			// (PRIMARY, student, id,name, T5)
			ddl_state.primary_keys = parseColumnList(q.arg2);

			// �������CreateTable���ӣ���������
			if (symbolTables.find("T4") != symbolTables.end()) { // ����T4��CREATE_TABLE�Ľ��
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setPrimaryKeys(ddl_state.primary_keys);
				}
			}
		}
		else if (q.op == "NOT_NULL") {
			// (NOT_NULL, student, name,age, T6)
			ddl_state.not_null_columns = parseColumnList(q.arg2);

			// ����������ȷ����CREATE_TABLE����ADD_COLUMN
			if (symbolTables.find("T4") != symbolTables.end()) { // CREATE_TABLE����
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setNotNullColumns(ddl_state.not_null_columns);
				}
			}
			else { // ADD_COLUMN��������Ҫ���������AddColumn����
				for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
					AddColumn* add_op = dynamic_cast<AddColumn*>(it->second);
					if (add_op) {
						add_op->setNotNullColumns(ddl_state.not_null_columns);
						break;
					}
				}
			}
		}
		else if (q.op == "UNIQUE") {
			// (UNIQUE, student, id,phone, T7)
			ddl_state.unique_columns = parseColumnList(q.arg2);

			// ����������ȷ����CREATE_TABLE����ADD_COLUMN
			if (symbolTables.find("T4") != symbolTables.end()) { // CREATE_TABLE����
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setUniqueColumns(ddl_state.unique_columns);
				}
			}
			else { // ADD_COLUMN����
				for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
					AddColumn* add_op = dynamic_cast<AddColumn*>(it->second);
					if (add_op) {
						add_op->setUniqueColumns(ddl_state.unique_columns);
						break;
					}
				}
			}
		}

		// ====== ɾ������� ======
		else if (q.op == "CHECK_TABLE") {
			// (CHECK_TABLE, "employees", _, T1)
			// ������Խ��б�����Լ�飬��ͨ����DropTable�����д���
		}
		else if (q.op == "DROP_TABLE") {
			// (DROP_TABLE, "employees", _, T2)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for DROP_TABLE operation");
			}

			string table_name = q.arg1;
			bool if_exists = true; // ������CHECK_TABLE����IF EXISTS
			DropTable* drop_op = new DropTable(catalog, table_name, if_exists);
			symbolTables[q.result] = drop_op;
			root = drop_op; // DDL����ͨ���Ǹ��ڵ�
		}

		// ====== �������� ======
		else if (q.op == "ADD_COLUMN") {
			// (ADD_COLUMN, "students", T3, T4)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for ADD_COLUMN operation");
			}

			string table_name = q.arg1;
			AddColumn* add_op = new AddColumn(catalog, table_name, ddl_state.column_specs);
			symbolTables[q.result] = add_op;
			root = add_op; // DDL����ͨ���Ǹ��ڵ�
		}

		// ====== ɾ������� ======
		else if (q.op == "DROP_COLUMN") {
			// (DROP_COLUMN, "students", "sex", T1)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for DROP_COLUMN operation");
			}

			string table_name = q.arg1;
			vector<string> columns_to_drop = parseColumnList(q.arg2);
			DropColumn* drop_op = new DropColumn(catalog, table_name, columns_to_drop);
			symbolTables[q.result] = drop_op;
			root = drop_op; // DDL����ͨ���Ǹ��ڵ�
		}

		// ====== ������ ======
		else if (q.op == "RESULT") {
			return root;
		}
	}

	return root;
}

vector<Row> IndexScan::execute() {
	vector<Row> output;

	// ��������Ŀǰֻ֧�ֵ���������
	vector<string> cols = { column };
	vector<RID> results = indexManager->Search(tableName, cols, value);

	// ���� RID ��λ��¼
	for (auto& rid : results) {
		Page* page = bpm->fetchPage(rid.page_id);
		if (!page) continue;

		string record_str = page->readRecord(rid.slot_id);
		if (record_str.empty()) continue;

		Row row;
		vector<string> parts;
		split(record_str, ",", parts);
		for (const auto& p : parts) {
			size_t colon_pos = p.find(":");
			if (colon_pos != string::npos) {
				string key = p.substr(0, colon_pos);
				string val = p.substr(colon_pos + 1);
				row[key] = val;
			}
		}

		output.push_back(row);
		bpm->unpinPage(rid.page_id, false);
	}

	return output;
}