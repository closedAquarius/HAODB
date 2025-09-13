#include "executor.h"

using namespace std;

Scan::Scan(BufferPoolManager* bpm, const string& tName) : bpm(bpm), tableName(tName) {}
vector<Row> Scan::execute() {
	// TODO: 从元数据中获得tableName的页地址
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
	// TODO: 从元数据中获得tableName的页地址
	// 假设Students表位于 PageId 0
	PageId pageId = 0;

	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for insert.");
	}

	// 将每一行数据插入到页中
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

	bpm->unpinPage(pageId, true); // 标记为脏页
	return {}; // INSERT不返回数据
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
	// 获取子节点（通常是 Filter）返回的行
	vector<Row> rowsToUpdate = child->execute();
	vector<Row> updateData = data->execute();
	vector<Row> output;

	Row updateValues = updateData.front();
	// TODO
	// 假设在 Scan 或 Filter 阶段已经将物理地址（例如：PageId 和 SlotId）存储在 Row 对象中
	// 实际系统中，Row 对象会包含 `rid`（Record ID），用来定位记录
	// 简化，直接从 child 获得 Row，然后假设可以定位并更新

	for (auto& row : rowsToUpdate) {
		// 伪代码：从 Row 获取其物理地址 (pageId, slotId)
		// PageId pageId = row.getPageId();
		// SlotId slotId = row.getSlotId();

		// 假设通过某种机制可以获取到页号，这里我们假设为 0
		PageId pageId = 0; // 这是一个简化！

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

		// 将新记录写入页面的槽中
		page->getSlot(0)->length = 0;
		page->insertRecord(updatedRecordStr.c_str(), updatedRecordStr.size());

		// 将页面标记为脏，以便后续写回磁盘
		bpm->unpinPage(pageId, true); // true 表示该页是脏页

		// 手动写回
		bpm->flushPage(pageId);

		output.push_back(row);
	}

	return output;
}

Delete::Delete(Operator* c, BufferPoolManager* b, const string& tName) : child(c), bpm(b), tableName(tName) {}
vector<Row> Delete::execute() {
	// child 算子树执行后，返回需要删除的行。
	vector<Row> inputRows = child->execute();
	if (inputRows.empty()) {
		cout << "No rows found to delete.\n";
		return {};
	}

	// TODO: 找到 tableName 对应的页，这里简化为 PageId 0
	PageId pageId = 0;
	Page* page = bpm->fetchPage(pageId);
	if (!page) {
		throw runtime_error("Failed to fetch page for delete.");
	}

	// 遍历所有需要删除的行
	for (auto& row : inputRows) {
		// 找到这行在页中的位置，并进行逻辑删除
		// 同样，这里采用低效的遍历查找方式。
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

	bpm->unpinPage(pageId, true); // 标记为脏页
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
			// arg1 是 Value 算子的符号，arg2 是表名
			Operator* child = symbolTables[q.arg1];
			root = new Insert(child, bpm, q.arg2);
		}
		// ====== SET ======
		else if (q.op == "SET") {
			// 假设 SET 四元式的格式为 (SET, 列名, 值, 符号)
			symbolTables[q.result] = new Set(q.arg1, q.arg2);
		}
		// ====== 更新 ======
		else if (q.op == "UPDATE") {
			Operator* child = symbolTables[q.arg1];
			Operator* data = symbolTables[q.arg2];
			root = new Update(child, data, bpm);
		}

		// ====== 删除 ======
		else if (q.op == "DELETE") {
			// arg1 是 FROM 或 WHERE 算子的符号，用于定位要删除的行
			// q.result 是表名
			Operator* child = symbolTables[q.arg1]; // 获取 FROM/WHERE 算子
			root = new Delete(child, bpm, q.result);
		}

		// ====== 输出结果 ======
		else if (q.op == "RESULT") {
			return root;
		}
	}

	return root;
}
