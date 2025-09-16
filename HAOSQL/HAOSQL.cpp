// HAOSQL.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // VS 需要

#include "dataType.h"
#include "parser.h"
#include "lexer.h"
#include "semantic.h"
#include "executor.h"
#include "buffer_pool.h";
#include "B+tree.h"
#include "AI.h"
#include "rollback.h"
using namespace std;

vector<Quadruple> sql_compiler(string sql);
vector<Quadruple> sql_compiler(string sql, SOCKET clientSock);
int generateDBFile();
int addDBFile();
void initIndexManager(CatalogManager* catalog);
bool checkDatabaseExists(string sql, CatalogManager& catalog, SOCKET clientSock);
bool checkDatabaseExists(string sql, CatalogManager& catalog);
bool checkRollBack(string sql, CatalogManager& catalog, BufferPoolManager* bpm, SOCKET clientsocket);
//void printRollBackRecords(vector<WithdrawLog> logs, int steps, CatalogManager& catalog);
std::string getRollBackRecordsString(const std::vector<WithdrawLog>& logs, int steps, CatalogManager& catalog);
IndexManager* indexManager = nullptr;

// 全局变量
std::string USER_NAME;
void SET_USER_Name(std::string& name)
{
    USER_NAME = name;
}
SQLTimer GLOBAL_TIMER;

// 工具函数：封装发送，自动加 >>END
void sendWithEnd(SOCKET sock, const string& msg) {
    string data = msg + ">>END\n";
    send(sock, data.c_str(), static_cast<int>(data.size()), 0);
}

// 每个客户端一个线程
void handle_client(SOCKET clientSock, sockaddr_in clientAddr) {

    // 加载元数据
    auto catalog = std::make_unique<CatalogManager>("HAODB");
    //CatalogManager catalog("HAODB");
    catalog->Initialize();

    char buffer[4096];
    string account, password;


    // 登录循环
    while (true) {
        sendWithEnd(clientSock, "请输入账号:");

        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            closesocket(clientSock);
            return;
        }
        account = buffer;
        std::cout << "[客户端账号] " << account << endl;

        sendWithEnd(clientSock, "请输入密码:");

        memset(buffer, 0, sizeof(buffer));
        bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            closesocket(clientSock);
            return;
        }
        password = buffer;
        std::cout << "[客户端密码] " << password << endl;

        FileManager fm("HAODB");
        LoginManager lm(fm, "HAODB");
        if (lm.loginUser(account, password)) {
            SET_USER_Name(account);
            std::string welcome =
                "-------------------------------------------------------------------------------\n"
                "- 名称规范 : 数据库名、表名、列名只能包含字母、数字和下划线，长度不超过63字符 -\n"
                "- 数据类型 : 支持 \"INT\", \"VARCHAR\", \"DECIMAL\", \"DATETIME\"                     -\n"
                "- 主键约束 : 主键列自动设为不可空                                             -\n"
                "- 帮助 : exit退出数据库系统                                                   -\n"
                "-------------------------------------------------------------------------------\n"
                "欢迎您 " + current_username;
            sendWithEnd(clientSock, welcome);
            break;
        }
        else {
            sendWithEnd(clientSock, "账号或密码错误，请重试");
        }
    }

    setDBName("System_Data");
    initIndexManager(catalog.get());
    StorageConfigInfo info = catalog->GetStorageConfig(DBName);

    // 创建 DiskManager
    DiskManager dm(info.data_file_path);
    // 创建 BufferPoolManager
    // 缓冲池大小 10
    BufferPoolManager bpm(10, &dm);
    // SQL 循环
    while (true) {


        sendWithEnd(clientSock, "请输入 SQL 语句:");

        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            break;
        }

        string sql = buffer;
        if (checkDatabaseExists(sql, *catalog,clientSock) || checkRollBack(sql, *catalog, &bpm,clientSock))
            continue;
        string lowerSql = sql;
        transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);

        if (sql == "exit") break;
        if (sql == "gen") {
            generateDBFile();
            continue;
        }
        if (sql == "add") {
            addDBFile();
            continue;
        }
        if (sql.empty()) {
            continue;
        }
        const string prefix = "use database ";
        if (lowerSql.find(prefix) == 0) {
            string dbName = sql.substr(prefix.size()); // 保留原大小写
            // 去掉末尾分号
            if (!dbName.empty() && dbName.back() == ';') dbName.pop_back();
            // 去掉首尾空格
            dbName.erase(0, dbName.find_first_not_of(" \t"));
            dbName.erase(dbName.find_last_not_of(" \t") + 1);
            setDBName(dbName);
            initIndexManager(catalog.get());
            sendWithEnd(clientSock, "正在使用"+dbName);
            continue;
        }

        // 编译SQL，开始计时
        GLOBAL_TIMER.start();

        std::vector<Quadruple> quadruple;
        try {
            quadruple = sql_compiler(sql,clientSock);

            if (quadruple.empty()) {
                std::cerr << "Error: SQL parsing failed, please check syntax." << std::endl;
                continue;
            }

            std::cout << "Generated quadruples:" << std::endl;
            for (auto& q : quadruple) {
                std::cout << "(" << q.op << ", "
                    << q.arg1 << ", "
                    << q.arg2 << ", "
                    << q.result << ")" << std::endl;
            }

            SET_SQL_QUAS(sql, quadruple);
        }
        catch (const std::exception& e) {
            std::cerr << "Error (compile): " << e.what() << std::endl;
            continue;  // 允许用户继续输入
        }

        // ----------------------
        // 执行阶段错误处理
        // ----------------------
        try {
            vector<string> columns;
            Operator* root = buildPlan(quadruple, columns, &bpm, catalog.get());
            vector<Row> result = root->execute();

            string resultStr;
            if (!result.empty()) {
                std::cout << "主函数打印" << endl;
                for (auto& row : result) {
                    for (auto& col : columns) {
                        std::cout << row.at(col) << "\t|";
                        resultStr += row.at(col) + "\t|";
                    }
                    std::cout << endl;
                    resultStr += "\n";
                }
            }

            if (resultStr.empty()) {
                sendWithEnd(clientSock, "执行成功！");
            }
            else {
                sendWithEnd(clientSock, resultStr);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "执行错误: " << e.what() << std::endl;
            sendWithEnd(clientSock, string("执行错误: ") + e.what());
            continue;  // 不中断主循环
        }
    }

    closesocket(clientSock);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        cerr << "socket failed, error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "bind failed, error: " << WSAGetLastError() << endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "listen failed, error: " << WSAGetLastError() << endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::cout << "服务器已启动，端口 8080，等待客户端连接..." << endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);
        if (clientSock == INVALID_SOCKET) {
            cerr << "accept failed, error: " << WSAGetLastError() << endl;
            continue;
        }

        thread(handle_client, clientSock, clientAddr).detach();
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}

/*int main()
{
    SetConsoleOutputCP(CP_ACP);   // 控制台输出 UTF-8
    SetConsoleCP(CP_ACP);         // 控制台输入 UTF-8
    std::cout << "Hello World!\n";
    string sql;
    
    // 加载元数据
    auto catalog = std::make_unique<CatalogManager>("HAODB");
    //CatalogManager catalog("HAODB");
    catalog->Initialize();
    setDBName("T");
    initIndexManager(catalog.get());

    /*
    cout << catalog.GetStorageConfig(DBName).data_file_path << endl
        << catalog.GetStorageConfig(DBName).index_file_path << endl
        << catalog.GetStorageConfig(DBName).log_file_path << endl;

    StorageConfigInfo info = catalog->GetStorageConfig(DBName);

    cout << endl
        << "------------------------------------------------------------------------------- " << endl
        << "- 名称规范 : 数据库名、表名、列名只能包含字母、数字和下划线，长度不超过63字符 -" << endl
        << "- 数据类型 : 支持 \"INT\", \"VARCHAR\", \"DECIMAL\", \"DATETIME\"                     -" << endl
        << "- 主键约束 : 主键列自动设为不可空                                             -" << endl
        << "- 帮助 : exit退出数据库系统                                                   -" << endl
        << "------------------------------------------------------------------------------- " << endl;

    while (true)
    {
        // 创建 DiskManager
        DiskManager dm(info.data_file_path);
        // 创建 BufferPoolManager
        // 缓冲池大小 10
        BufferPoolManager bpm(10, &dm);
        
        cout << "请输入 SQL 语句: ";
        getline(cin, sql);
        cout << sql << endl;

        if(checkDatabaseExists(sql, *catalog) || checkRollBack(sql, *catalog, &bpm))
            continue;

        if (sql == "exit") break;
        if (sql == "gen") {
            generateDBFile();
            continue;
        }
        if (sql == "add") {
            addDBFile();
            continue;
        }
        if (sql.empty()) {
            continue;
        }
         
        /*std::string correctedSQL;
        try {
            // correctedSQL = CALLAI(sql);
        } catch (...) {
            std::cerr << "AI调用失败，继续执行原 SQL" << std::endl;
        }

        std::string finalSQL = correctedSQL.empty() ? sql : correctedSQL;
        std::cout << "最终执行 SQL: " << finalSQL << std::endl;

        // 编译SQL，开始计时
        GLOBAL_TIMER.start();

        vector<Quadruple> quadruple = sql_compiler(sql);
        SET_SQL_QUAS(sql , quadruple);

        //vector<TableInfo> tables = catalog.ListTables("HelloDB");
        //cout << "!!!!!!!!!!!!" << endl;
        //cout << "表数量" << catalog.GetDatabaseInfo("HelloDB").table_count << endl;
        //catalog.ShowTableList("HelloDB");
        //for (auto& table : tables)
        //{
        //    cout << table.table_name << " " << table.data_file_offset << endl;
        //}

        // 构建并执行计划
        vector<string> columns;
        Operator* root = buildPlan(quadruple, columns, &bpm, catalog.get());
        vector<Row> result = root->execute();

        if (!result.empty()) {
            cout << "主函数打印" << endl;
            // 输出结果
            cout << endl;
            for (auto& col : columns) {
                cout << col << "\t|";
            }
            cout << endl;
            for (auto& row : result) {
                for (auto& col : columns) {
                    cout << row.at(col) << "\t|";
                }
                cout << endl;
            }
        }
    }

    return 0;
}*/

bool checkDatabaseExists(string sql, CatalogManager& catalog)
{
    // 将sql转换为小写以进行大小写不敏感的比较
    std::string lower_sql = sql;
    std::transform(lower_sql.begin(), lower_sql.end(), lower_sql.begin(), ::tolower);

    // 查找"create database"子字符串
    std::string keyword = "create database";
    size_t pos = lower_sql.find(keyword);

    if (pos != std::string::npos) {
        // 找到 "CREATE DATABASE"，提取数据库名
        size_t start_pos = pos + keyword.length();

        // 忽略空白字符，查找数据库名的起始位置
        while (start_pos < sql.length() && std::isspace(sql[start_pos])) {
            ++start_pos;
        }

        // 查找数据库名的结束位置（直到下一个空白或字符串结束）
        size_t end_pos = start_pos;
        while (end_pos < sql.length() && !std::isspace(sql[end_pos]) && sql[end_pos] != ';') {
            ++end_pos;
        }

        // 提取数据库名
        std::string db_name = sql.substr(start_pos, end_pos - start_pos);
        setDBName(db_name);
        catalog.CreateDatabase(DBName, current_username);


        return true;
    }

    return false;
}

bool checkRollBack(string sql, CatalogManager& catalog, BufferPoolManager* bpm,SOCKET clientsocket)
{
    // 转换为小写，便于大小写不敏感匹配
    std::string lower_sql = sql;
    std::transform(lower_sql.begin(), lower_sql.end(), lower_sql.begin(), ::tolower);

    // 查找 "rollback"
    std::string keyword = "rollback";
    size_t pos = lower_sql.find(keyword);

    if (pos != std::string::npos) {
        size_t start_pos = pos + keyword.length();

        // 跳过空格
        while (start_pos < sql.length() && std::isspace(sql[start_pos])) {
            ++start_pos;
        }

        // 提取数字
        size_t end_pos = start_pos;
        while (end_pos < sql.length() && std::isdigit(sql[end_pos])) {
            ++end_pos;
        }

        if (start_pos == end_pos) {
            return false; // 没有数字
        }

        std::string num_str = sql.substr(start_pos, end_pos - start_pos);

        // 去掉可能的分号
        if (!num_str.empty() && num_str.back() == ';') {
            num_str.pop_back();
        }

        int steps = std::stoi(num_str);
        EnhancedExecutor enhanced_executor("HAODB", USER_NAME, "");
        // enhanced_executor.Initialize();

        // 回滚
        auto logs = enhanced_executor.UndoLastOperation(steps);

        enhanced_executor.PrintAllWAL();

        string back = getRollBackRecordsString(logs, steps, catalog);

        sendWithEnd(clientsocket, back + "\n是否撤销(Y / N) : ");

        // 接收用户输入
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        vector<Log> undoLogs;
        int bytesReceived = recv(clientsocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';  // 以字符串形式处理
            std::string input(buffer);

            // 去掉可能的换行符
            input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
            input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

            if (input == "Y" || input == "y") {
                for (auto& log : logs)
                {
                    Log undoLog;
                    undoLog.pageId = log.page_id;
                    undoLog.slotId = log.slot_id;
                    undoLog.len = log.length;
                    undoLog.type = log.record_type;

                    undoLogs.push_back(undoLog);
                }

                undos(undoLogs, bpm);
                back = "撤销成功！";
                sendWithEnd(clientsocket, back);
            }
            else {
                // 用户选择不撤销
                back = "撤销失败！";
                sendWithEnd(clientsocket, back);
            }
        }
        return true;
    }
    return false;
}
std::string getRollBackRecordsString(const std::vector<WithdrawLog>& logs, int steps, CatalogManager& catalog)
{
    std::ostringstream oss;
    //LogViewer log_viewer("HAODB", &catalog); // 之前带 catalog 的，如果用不到可以去掉
    LogViewer log_viewer("HAODB");

    oss << "-------------------------------\n";
    oss << "最近" << steps << "步的操作为：\n";

    auto operations = log_viewer.GetRecentOperations(steps);
    if (operations.empty()) {
        oss << "  [INFO] No operation logs found.\n";
        oss << "  Tip: Execute some database operations first to generate logs.\n";
        oss << std::string(80, '=') << "\n";
        return oss.str();
    }

    oss << " Line | Level | Content\n";
    oss << std::string(80, '-') << "\n";

    int line_num = 1;
    for (const auto& op : operations) {
        // 格式化输出每一行
        oss << std::setw(5) << line_num << " | ";

        // 根据内容判断操作类型并添加标识
        if (op.find("INSERT") != std::string::npos) {
            oss << "ADD  | ";
        }
        else if (op.find("DELETE") != std::string::npos) {
            oss << "DEL  | ";
        }
        else if (op.find("UPDATE") != std::string::npos) {
            oss << "UPD  | ";
        }
        else if (op.find("SELECT") != std::string::npos) {
            oss << "SEL  | ";
        }
        else if (op.find("CREATE") != std::string::npos) {
            oss << "CRT  | ";
        }
        else {
            oss << "OTH  | ";
        }

        // 截断过长的日志行以便显示
        if (op.length() > 65) {
            oss << op.substr(0, 62) << "..." << "\n";
        }
        else {
            oss << op << "\n";
        }

        line_num++;
    }

    oss << std::string(80, '-') << "\n";
    return oss.str();
}
/*void printRollBackRecords(vector<WithdrawLog> logs, int steps, CatalogManager& catalog)
{
    //INIT_LOGGER("HAODB", &catalog);
    LogViewer log_viewer("HAODB");
    
    cout << "-------------------------------" << endl;
    cout << "最近" << steps << "步的操作为：" << endl;

    auto operations = log_viewer.GetRecentOperations(steps);
    if (operations.empty()) {
        std::cout << "  [INFO] No operation logs found." << std::endl;
        std::cout << "  Tip: Execute some database operations first to generate logs." << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        return;
    }

    std::cout << " Line | Level | Content" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    int line_num = 1;
    for (const auto& op : operations) {
        // 格式化输出每一行
        std::cout << std::setw(5) << line_num << " | ";

        // 根据内容判断操作类型并添加标识
        if (op.find("INSERT") != std::string::npos) {
            std::cout << "ADD  | ";
        }
        else if (op.find("DELETE") != std::string::npos) {
            std::cout << "DEL  | ";
        }
        else if (op.find("UPDATE") != std::string::npos) {
            std::cout << "UPD  | ";
        }
        else if (op.find("SELECT") != std::string::npos) {
            std::cout << "SEL  | ";
        }
        else if (op.find("CREATE") != std::string::npos) {
            std::cout << "CRT  | ";
        }
        else {
            std::cout << "OTH  | ";
        }

        // 截断过长的日志行以便显示
        if (op.length() > 65) {
            std::cout << op.substr(0, 62) << "..." << std::endl;
        }
        else {
            std::cout << op << std::endl;
        }

        line_num++;
    }

    std::cout << std::string(80, '-') << std::endl;
}*/

bool checkDatabaseExists(string sql, CatalogManager& catalog, SOCKET clientSock)
{
    // 将sql转换为小写以进行大小写不敏感的比较
    std::string lower_sql = sql;
    std::transform(lower_sql.begin(), lower_sql.end(), lower_sql.begin(), ::tolower);

    // 查找"create database"子字符串
    std::string keyword = "create database";
    size_t pos = lower_sql.find(keyword);

    if (pos != std::string::npos) {
        // 找到 "CREATE DATABASE"，提取数据库名
        size_t start_pos = pos + keyword.length();

        // 忽略空白字符，查找数据库名的起始位置
        while (start_pos < sql.length() && std::isspace(sql[start_pos])) {
            ++start_pos;
        }

        // 查找数据库名的结束位置（直到下一个空白或字符串结束）
        size_t end_pos = start_pos;
        while (end_pos < sql.length() && !std::isspace(sql[end_pos]) && sql[end_pos] != ';') {
            ++end_pos;
        }

        // 提取数据库名
        std::string db_name = sql.substr(start_pos, end_pos - start_pos);
        setDBName(db_name);
        try {
            catalog.CreateDatabase(DBName, current_username);
            sendWithEnd(clientSock, "数据库 " + db_name + " 创建成功！");
        }
        catch (const std::exception& e) {
            sendWithEnd(clientSock, string("创建数据库失败: ") + e.what());
        }

        return true;
    }

    return false;
}

void initIndexManager(CatalogManager* catalog) {
    // 假设你有 FileManager 和 CatalogManager
    FileManager fm("./HAODB/"+DBName+"/index");
    cout << "initIndexManager" << endl;
    std::cout << "检查catalog对象:" << std::endl;
    std::cout << "catalog指针: " << catalog << std::endl;
    std::cout << "this指针: " << static_cast<void*>(catalog) << std::endl;

    // 尝试访问成员变量
    try {
        // 如果对象有效，这应该正常工作
        std::cout << "对象状态检查通过" << std::endl;
    }
    catch (...) {
        std::cout << "对象状态检查失败 - 对象可能已损坏" << std::endl;
    }
    indexManager = new IndexManager(fm, catalog);
}
vector<Quadruple> sql_compiler(string sql, SOCKET clientSock)
{
    Lexer lexer(sql);
    SQLParser sqlParser;
    try {
        cout << "开始词法分析" << endl;
        vector<Token> tokens = lexer.analyze();

        for (auto& t : tokens) {
            cout << left << setw(10) << t.type
                << setw(15) << t.value
                << setw(10) << t.line
                << setw(10) << t.column << endl;
        }

        for (int i = 0; i < tokens.size(); ++i) {
            std::cout << tokens[i].type << " " << tokens[i].value << " ";
        }
        cout << "开始语法分析" << endl;
        sqlParser.start_parser(tokens);

        SemanticAnalyzer analyzer;
        auto quads = analyzer.analyze(tokens);

        std::cout << "四元式结果：" << std::endl;
        for (auto& q : quads) {
            std::cout << "(" << q.op << ", " << q.arg1 << ", "
                << q.arg2 << ", " << q.result << ")" << std::endl;
        }

        return quads;
    }
    catch (const LexicalError& e) {
        std::cout << e.what() << std::endl;
        std::string correctedSQL;
        try {
            correctedSQL = CALLAI(sql);
        }
        catch (...) {
            std::cerr << "AI调用失败，继续执行原 SQL" << std::endl;
            string back = std::string(e.what());
            sendWithEnd(clientSock, back);
            //send(clientSock, back.c_str(), static_cast<int>(back.size()), 0);
            return {};
        }
        if (correctedSQL.empty()) {
            return {};
        }
        else {
            sql = correctedSQL;
            string back = std::string(e.what())+ "\n修正SQL: " + sql;
            send(clientSock, back.c_str(), static_cast<int>(back.size()), 0);
        }
        
        std::cout << "最终执行 SQL: " << sql << std::endl;
        return sql_compiler(sql, clientSock);
    }
    catch (const SemanticError& e) {
        std::cout << "语义分析错误：" << e.what() << std::endl;
        std::string correctedSQL;
        try {
            correctedSQL = CALLAI(sql);
        }
        catch (...) {
            std::cerr << "AI调用失败，继续执行原 SQL" << std::endl;
            string back = std::string(e.what())
                + " at line " + std::to_string(e.line)
                + ", column " + std::to_string(e.col);
            sendWithEnd(clientSock, back);
            //send(clientSock, back.c_str(), static_cast<int>(back.size()), 0);
            return {};
        }
        if (correctedSQL.empty()) {
            return {};
        }
        else {
            sql = correctedSQL;
            string back = std::string(e.what())
                + " at line " + std::to_string(e.line)
                + ", column " + std::to_string(e.col)
                + "\n修正SQL: " + sql;
            send(clientSock, back.c_str(), static_cast<int>(back.size()), 0);
        }
        std::cout << "最终执行 SQL: " << sql << std::endl;
        return sql_compiler(sql, clientSock);
    }
    catch (const std::exception& e) {
        std::cerr << "[Syntax Error] " << e.what() << std::endl;
        return {};  // 返回空，表示错误
    }
}
vector<Quadruple> sql_compiler(string sql)
{
    Lexer lexer(sql);
    SQLParser sqlParser;
    try {
        cout << "开始词法分析" << endl;
        vector<Token> tokens = lexer.analyze();

        for (auto& t : tokens) {
            cout << left << setw(10) << t.type
                << setw(15) << t.value
                << setw(10) << t.line
                << setw(10) << t.column << endl;
        }

        for (int i = 0; i < tokens.size(); ++i) {
            std::cout << tokens[i].type << " " << tokens[i].value << " ";
        }
        cout << "开始语法分析" << endl;
        sqlParser.start_parser(tokens);

        SemanticAnalyzer analyzer;
        auto quads = analyzer.analyze(tokens);

        std::cout << "四元式结果：" << std::endl;
        for (auto& q : quads) {
            std::cout << "(" << q.op << ", " << q.arg1 << ", "
                << q.arg2 << ", " << q.result << ")" << std::endl;
        }

        return quads;
    }
    catch (const LexicalError& e) {
        /*
        std::cout << e.what() << std::endl;
        std::string correctedSQL;
        try {
            correctedSQL = CALLAI(sql);
        }
        catch (...) {
            std::cerr << "AI调用失败，继续执行原 SQL" << std::endl;
        }
        if (correctedSQL.empty()) {
            return {};
        }
        else {
            sql = correctedSQL;
            string back = std::string(e.what()) + "\n修正SQL: " + sql;
        }
        std::cout << "最终执行 SQL: " << sql << std::endl;
        return sql_compiler(sql);*/
    }
    catch (const SemanticError& e) {
        std::cout << "语义分析错误：" << e.what() << std::endl;
        std::string correctedSQL;
        try {
            correctedSQL = CALLAI(sql);
        }
        catch (...) {
            std::cerr << "AI调用失败，继续执行原 SQL" << std::endl;
        }
        if (correctedSQL.empty()) {
            return {};
        }
        else {
            sql = correctedSQL;
            string back = std::string(e.what()) + "\n修正SQL: " + sql;
        }
        std::cout << "最终执行 SQL: " << sql << std::endl;
        return sql_compiler(sql);
    }
    catch (const std::exception& e) {
        std::cerr << "[Syntax Error] " << e.what() << std::endl;
        return {};  // 返回空，表示错误
    }
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

// 生成模拟数据
//#include "page.h"
//#include <fstream>
//#include <iostream>
//
//using namespace std;
//
int generateDBFile() {
    DiskManager dm("HAODB/Students/data/students.dat");
    Page p(DATA_PAGE);

    // 假设这些是你的Students表数据
    vector<string> records = {
        "id:1,name:Alice,age:21,grade:A",
        "id:2,name:Bob,age:18,grade:B",
        "id:3,name:Charlie,age:22,grade:A",
        "id:4,name:John,age:19,grade:A"
    };

    // 插入记录到页中
    for (const auto& rec : records) {
        p.insertRecord(rec.c_str(), rec.length());
    }

    // 将页写回磁盘
    dm.writePage(0, p);

    return 0;
}

int addDBFile() {
    DiskManager dm("HAODB/Students/data/students.dat");
    Page p(DATA_PAGE);

    //// 假设这些是你的Students表数据
    //vector<string> records = {
    //    "id:1,name:1Alice,age:21,grade:A",
    //    "id:2,name:2Bob,age:18,grade:B",
    //    "id:3,name:3Charlie,age:22,grade:A",
    //    "id:4,name:4John,age:19,grade:A"
    //};

    //// 插入记录到页中
    //for (const auto& rec : records) {
    //    p.insertRecord(rec.c_str(), rec.length());
    //}

    // 将页写回磁盘
    dm.addPage(2, p);

    return 0;
}