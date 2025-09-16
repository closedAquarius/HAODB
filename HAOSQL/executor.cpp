#include "executor.h"

using namespace std;

// 全局变量
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

	// 通过缓冲池获取页
	Page* page = bpm->fetchPage(pageOffset);
	if (!page) {
		throw runtime_error("Failed ro fetch page from buffer pool.");
	}

	// 遍历页中的所有槽，解析记录
	for (int i = 0; i < page->header()->slot_count; i++) {
		string record_str = page->readRecord(i);
		Row row;

		// 记录是键值对的字符串形式，例如 "id:1,name:Alice,..."
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

// Values 算子实现
Values::Values(const std::string& v, const std::string& c) {
	// 解析值列表，例如 "(1,"test",19,"B")"
	// 去掉括号，并按逗号分割
	string trimmed_v = v.substr(1, v.length() - 2);
	split(trimmed_v, ",", this->values);

	// 解析列名列表，例如 "(id,name,age,grade)"
	string trimmed_c = c.substr(1, c.length() - 2);
	split(trimmed_c, ",", this->columns);

	if (this->values.size() != this->columns.size()) {
		throw std::runtime_error("Values and columns count mismatch.");
	}
}
vector<Row> Values::execute() {
	Row newRow;
	for (size_t i = 0; i < values.size(); ++i) {
		// 去掉值的引号，如果存在的话
		string val = values[i];
		if (val.front() == '"' && val.back() == '"') {
			val = val.substr(1, val.length() - 2);
		}
		newRow[columns[i]] = val;
	}
	return { newRow };
}

// Insert 算子实现
Insert::Insert(Operator* c, BufferPoolManager* b, const string& tName, PageId i) : child(c), bpm(b), tableName(tName), pageOffset(i) {}
vector<Row> Insert::execute() {
	// 从子节点（VALUES 算子）获取要插入的数据行
	vector<Row> input = child->execute();

	PageId pageId = pageOffset;

	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for insert.");
	}

	// 获取该表的所有索引信息
	// IndexManager* indexManager; // 假设全局可用
	std::cout << "initIndexManager=" << endl;
	std::cout << "检查catalog对象:" << std::endl;
	std::cout << "catalog指针: " << indexManager->catalog_ << std::endl;
	std::cout << "this指针: " << static_cast<void*>(indexManager->catalog_) << std::endl;

	// 尝试访问成员变量
	try {
		// 如果对象有效，这应该正常工作
		std::cout << "对象状态检查通过" << std::endl;
	}
	catch (...) {
		std::cout << "对象状态检查失败 - 对象可能已损坏" << std::endl;
	}
	vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable(tableName);

	// 将每一行数据插入到页中
	for (auto& row : input) {
		string record_str = "";
		bool first = true;
		for (const auto& pair : row) {
			if (!first) record_str += ",";
			record_str += pair.first + ":" + pair.second;
			first = false;
		}

		// 插入页并获得 RID
		int slotId = page->insertRecord(record_str.c_str(), record_str.length());
		RID rid{ pageId, slotId };

		// 插入所有索引
		for (auto& idx : tableIndexes) {
			std::cout << "===================================" << endl;
			std::cout << idx.column_names[0] << endl;
			int keyInt = 0;

			if (idx.column_names.size() == 1) {
				// 单列索引：直接转 int
				const string& col = idx.column_names[0];
				keyInt = std::stoi(row.at(col));
			}
			else {
				// 多列索引：拼接字符串后 hash 成 int
				string combinedKey;
				bool firstKey = true;
				for (auto& col : idx.column_names) {
					if (!firstKey) combinedKey += "|";  // 分隔符
					combinedKey += row.at(col);
					combinedKey += row.at(col);
					firstKey = false;
				}
				std::hash<std::string> hasher;
				keyInt = static_cast<int>(hasher(combinedKey));
			}
		}

		// 完成操作，结束计时
		GLOBAL_TIMER.stop();

		SET_AFTER_WAL_RECORD(pageId, slotId, page->getSlot(slotId)->length);
		// 记入日志
		enhanced_executor->InsertRecord(WAL_DATA_RECORD.after_page_id, WAL_DATA_RECORD.after_slot_id, WAL_DATA_RECORD.after_length,
			WAL_DATA_RECORD.sql, WAL_DATA_RECORD.quas, USER_NAME, true, GLOBAL_TIMER.elapsed_ms(), "");

		// enhanced_executor->PrintAllWAL();
	}

	bpm->unpinPage(pageId, true); // 标记为脏页
	bpm->flushPage(pageId);       // 手动写回

	return {}; // INSERT 不返回数据
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
		long val = std::stol(str, &pos, 10);  // 先用 long 接，避免越界
		if (pos != str.size()) return false;  // 有非法字符
		if (val < std::numeric_limits<int>::min() || val > std::numeric_limits<int>::max())
			return false;  // 超出 int 范围
		out = static_cast<int>(val);
		return true;
	}
	catch (...) {
		return false;  // 转换失败
	}
}

Update::Update(Operator* c, Operator* d, BufferPoolManager* b, PageId i) :child(c), data(d), bpm(b), pageOffset(i) {}
vector<Row> Update::execute() {
	// 获取子节点（通常是 Filter）返回的行
	vector<Row> rowsToUpdate = child->execute();
	vector<Row> updateData = data->execute();
	vector<Row> output;

	Row updateValues = updateData.front();

	for (auto& row : rowsToUpdate) {

		PageId pageId = pageOffset;

		// 锁定并获取页面
		Page* page = bpm->fetchPage(pageId);
		if (!page) {
			throw runtime_error("Failed to fetch page for update.");
		}

		// 修改行中的值
		for (const auto& kv : updateValues) {
			row[kv.first] = kv.second;
		}

		// 将修改后的行重新编码为字符串
		string updatedRecordStr = "";

		for (auto& [k, v] : row) {
			updatedRecordStr += (k + ":" + v + ",");
		}

		// 找到这行在页中的位置，并进行逻辑删除
		// 同样，这里采用遍历查找方式。
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

			// 记录老记录位置
			SET_BEFORE_WAL_RECORD(pageId, targetSlot, before_length);
		}
		else {
			std::cout << "Row with " << key_col << " = " << key_val << " not found." << endl;
		}
		vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable("students"); // TODO: 替换成当前表名
		for (auto& idx : tableIndexes) {
			int id = pageId;
			RID rid{ id, targetSlot };
			int i;
			if (tryParseInt(key_val, i)) {
				indexManager->DeleteEntry(idx.table_name, idx.column_names, i, rid);
			}
		}

		// 将新记录写入页面的槽中
		page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());
		string newKeyVal = row.at(key_col);
		int newKey = stoi(newKeyVal);
		for (auto& idx : tableIndexes) {
			int id = pageId;
			RID rid{ id, 0 }; // TODO: 根据 slot / 页号 定义 RID
			indexManager->InsertEntry(idx.table_name, idx.column_names, newKey, rid);
		}
		int newSlot =  page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());

		// 记录新纪录位置
		SET_AFTER_WAL_RECORD(pageId, newSlot, page->getSlot(newSlot)->length);

		// 将页面标记为脏，以便后续写回磁盘
		bpm->unpinPage(pageId, true); // true 表示该页是脏页

		// 手动写回
		bpm->flushPage(pageId);

		// 完成操作，结束计时
		GLOBAL_TIMER.stop();

		// 记入日志
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
	// child 算子树执行后，返回需要删除的行
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
	// 获取该表的所有索引信息
	vector<IndexInfo> tableIndexes = indexManager->FindIndexesByTable(tableName);

	// 遍历所有需要删除的行
	for (auto& row : inputRows) {
		string key_col = "id"; // 假设主键是 id
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
			// 构造 RID
			RID rid{ pageId, static_cast<uint16_t>(targetSlot) };

			// 删除所有相关索引
			for (auto& idx : tableIndexes) {
				int keyInt = 0;

				if (idx.column_names.size() == 1) {
					// 单列索引
					const string& col = idx.column_names[0];
					keyInt = std::stoi(row.at(col));
				}
				else {
					// 多列索引 → 组合成字符串再 hash
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

				// 调用 IndexManager 删除索引条目
				indexManager->DeleteEntry(tableName, idx.column_names, keyInt, rid);
			}

			// 删除物理记录
			page->deleteRecord(targetSlot);
			std::cout << "Deleted row with " << key_col << " = " << key_val << endl;

			// 完成操作，结束计时
			GLOBAL_TIMER.stop();

			// 记录老记录位置
			SET_BEFORE_WAL_RECORD(pageId, targetSlot, page->getSlot(targetSlot)->length);

			// 记入日志
			enhanced_executor->DeleteRecord(WAL_DATA_RECORD.before_page_id, WAL_DATA_RECORD.before_slot_id, WAL_DATA_RECORD.before_length,
				WAL_DATA_RECORD.sql, WAL_DATA_RECORD.quas, USER_NAME, true, GLOBAL_TIMER.elapsed_ms(), "");

			// enhanced_executor->PrintAllWAL();
		}
		else {
			std::cout << "Row with " << key_col << " = " << key_val << " not found." << endl;
		}
	}

	bpm->unpinPage(pageId, true); // 标记为脏页
	bpm->flushPage(pageId);       // 手动写回

	return {}; // DELETE 不返回数据
}


bool isNumber(const string& str) {
	stringstream ss(str);
	double num;
	return (ss >> num) && ss.eof();
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

// ========== DDL工具函数 ==========

vector<string> parseColumnList(const string& str) {
	vector<string> result;
	if (str.empty()) return result;

	size_t start = 0;
	size_t pos = 0;

	while (pos != string::npos) {
		pos = str.find(',', start);
		string token = str.substr(start, pos - start);

		// 去除空格
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
	// DDL构建状态
	DDLBuildState ddl_state;
	IndexedCondition ic;

	for (auto& q : quads) {
		// ========== 基本表扫描 ==========
		// (FROM, databaseName , - , T(scan))
		if (q.op == "FROM") {
			// 从元数据中获得tableName的页地址
			pageOffset = catalog->GetTableInfo(DBName, q.arg1).data_file_offset / 4096;
			symbolTables[q.result] = new Scan(bpm, q.arg1, pageOffset);
		}

		// ====== 条件选择 ======
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

		// ====== 条件选择 ======
		// (WHERE, T(scan) , T(condition) , T(filter))
		else if (q.op == "WHERE") {
			// 动态获取 FROM 节点，而不是写死 "T1"
			Operator* child = symbolTables[q.arg1];  // arg1 是 FROM 的符号
			Condition cond = condTable[q.arg2];      // arg2 是条件符号
			symbolTables[q.result] = new Filter(child, cond.get()); // 生成 Filter 节点
			

			// 提取条件信息（目前只支持 = 且单列索引）
			string colName = ic.col;
			string colValue = ic.val;
			string op = ic.op;

			// 从 FROM 节点里拿表名
			Scan* scanOp = dynamic_cast<Scan*>(child);
			if (!scanOp) {
				throw runtime_error("WHERE's child is not a Scan operator.");
			}
			string tableName = scanOp->tableName;

			extern IndexManager* indexManager;
			// 查找该表的索引
			vector<IndexInfo> indexes = indexManager->FindIndexesByTable(tableName);

			bool useIndex = false;
			for (auto& idx : indexes) {
				if (idx.column_names.size() == 1 && idx.column_names[0] == colName && op == "=") {
					// 命中索引
					int keyVal = stoi(colValue);
					symbolTables[q.result] = new IndexScan(bpm, tableName, colName, keyVal);
					useIndex = true;
					break;
				}
			}

			if (!useIndex) {
				// 回退到 Filter+Scan
				symbolTables[q.result] = new Filter(child, cond.get());
			}
		}

		// ====== 投影 ======
		// (SELECT , columns , T(filter) , T(Project))
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

		// ====== VALUES ======
		// (VALUES , values , columns , T(values))
		else if (q.op == "VALUES") {
			// arg1 是值列表字符串，arg2 是列名列表字符串
			// 创建 VALUES 算子，并将它存入符号表
			symbolTables[q.result] = new Values(q.arg1, q.arg2);
		}

		// ====== 插入 ======
		// (INSERT , T(scan) , T(values) , T(r))
		else if (q.op == "INSERT") {
			// arg1 是 FROM 的符号（表名），arg2 是 VALUES 的符号
			Operator* fromChild = symbolTables[q.arg1];
			Operator* valuesChild = symbolTables[q.arg2];

			// 从 FROM 算子中提取表名
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
			// 假设 SET 四元式的格式为 (SET, 列名, 值, 符号)
			symbolTables[q.result] = new Set(q.arg1, q.arg2);
		}

		// ====== 更新 ======
		// (UPDATE , T(scan) , T(set) , T(r))
		else if (q.op == "UPDATE") {
			Operator* child = symbolTables[q.arg1];
			Operator* data = symbolTables[q.arg2];
			root = new Update(child, data, bpm, pageOffset);
		}

		// ====== 删除 ======
		// (DELETE , T(scan) , - , T(r))
		else if (q.op == "DELETE") {
			// arg1 是 FROM 或 WHERE 算子的符号，用于定位要删除的行
			Operator* child = symbolTables[q.arg2]; // 获取 FROM/WHERE 算子
			root = new Delete(child, bpm, pageOffset);
		}

		// ========== DDL操作 ==========

		// ====== 创建表相关 ======
		else if (q.op == "COLUMN_NAME") {
			// (COLUMN_NAME, id,name,age,phone, -, T1)
			ddl_state.column_names = parseColumnList(q.arg1);
		}
		else if (q.op == "COLUMN_TYPE") {
			// (COLUMN_TYPE, VARCHAR,VARCHAR,INT,VARCHAR, 10,10,-1,15, T2)
			ddl_state.column_types = parseColumnList(q.arg1);
			ddl_state.column_lengths = parseColumnList(q.arg2);

			// 处理-1的情况（INT类型默认长度）
			for (auto& length : ddl_state.column_lengths) {
				if (length == "-1") {
					length = "4"; // INT默认4字节
				}
			}
		}
		else if (q.op == "COLUMNS") {
			// (COLUMNS, T1, T2, T3) - 合并列信息
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

			// 计算表的页id
			int pageId = catalog->GetDatabaseInfo(DBName).table_count;

			CreateTable* create_op = new CreateTable(catalog, ddl_state.table_name, ddl_state.column_specs, pageId, bpm->getDM());
			symbolTables[q.result] = create_op;
			root = create_op;
		}
		else if (q.op == "PRIMARY") {
			// (PRIMARY, student, id,name, T5)
			ddl_state.primary_keys = parseColumnList(q.arg2);

			// 如果已有CreateTable算子，设置主键
			if (symbolTables.find("T4") != symbolTables.end()) { // 假设T4是CREATE_TABLE的结果
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setPrimaryKeys(ddl_state.primary_keys);
				}
			}
		}
		else if (q.op == "NOT_NULL") {
			// (NOT_NULL, student, name,age, T6)
			ddl_state.not_null_columns = parseColumnList(q.arg2);

			// 根据上下文确定是CREATE_TABLE还是ADD_COLUMN
			if (symbolTables.find("T4") != symbolTables.end()) { // CREATE_TABLE场景
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setNotNullColumns(ddl_state.not_null_columns);
				}
			}
			else { // ADD_COLUMN场景，需要查找最近的AddColumn算子
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

			// 根据上下文确定是CREATE_TABLE还是ADD_COLUMN
			if (symbolTables.find("T4") != symbolTables.end()) { // CREATE_TABLE场景
				CreateTable* create_op = dynamic_cast<CreateTable*>(symbolTables["T4"]);
				if (create_op) {
					create_op->setUniqueColumns(ddl_state.unique_columns);
				}
			}
			else { // ADD_COLUMN场景
				for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
					AddColumn* add_op = dynamic_cast<AddColumn*>(it->second);
					if (add_op) {
						add_op->setUniqueColumns(ddl_state.unique_columns);
						break;
					}
				}
			}
		}

		// ====== 删除表相关 ======
		else if (q.op == "CHECK_TABLE") {
			// (CHECK_TABLE, "employees", _, T1)
			// 这里可以进行表存在性检查，但通常在DropTable算子中处理
		}
		else if (q.op == "DROP_TABLE") {
			// (DROP_TABLE, "employees", _, T2)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for DROP_TABLE operation");
			}

			string table_name = q.arg1;
			bool if_exists = true; // 假设有CHECK_TABLE就是IF EXISTS
			DropTable* drop_op = new DropTable(catalog, table_name, if_exists);
			symbolTables[q.result] = drop_op;
			root = drop_op; // DDL操作通常是根节点
		}

		// ====== 添加列相关 ======
		else if (q.op == "ADD_COLUMN") {
			// (ADD_COLUMN, "students", T3, T4)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for ADD_COLUMN operation");
			}

			string table_name = q.arg1;
			AddColumn* add_op = new AddColumn(catalog, table_name, ddl_state.column_specs);
			symbolTables[q.result] = add_op;
			root = add_op; // DDL操作通常是根节点
		}

		// ====== 删除列相关 ======
		else if (q.op == "DROP_COLUMN") {
			// (DROP_COLUMN, "students", "sex", T1)
			if (!catalog) {
				throw runtime_error("CatalogManager is required for DROP_COLUMN operation");
			}

			string table_name = q.arg1;
			vector<string> columns_to_drop = parseColumnList(q.arg2);
			DropColumn* drop_op = new DropColumn(catalog, table_name, columns_to_drop);
			symbolTables[q.result] = drop_op;
			root = drop_op; // DDL操作通常是根节点
		}

		// ====== 输出结果 ======
		else if (q.op == "RESULT") {
			return root;
		}
	}

	return root;
}

vector<Row> IndexScan::execute() {
	vector<Row> output;

	// 查索引（目前只支持单列索引）
	vector<string> cols = { column };
	vector<RID> results = indexManager->Search(tableName, cols, value);

	// 根据 RID 定位记录
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