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
#include "buffer_pool.h"
#include "login.h"

using namespace std;

vector<Quadruple> sql_compiler(string sql);

// 工具函数：封装发送，自动加 >>END
void sendWithEnd(SOCKET sock, const string& msg) {
    string data = msg + ">>END\n";
    send(sock, data.c_str(), static_cast<int>(data.size()), 0);
}

// 每个客户端一个线程
void handle_client(SOCKET clientSock, sockaddr_in clientAddr) {
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
        cout << "[客户端账号] " << account << endl;

        sendWithEnd(clientSock, "请输入密码:");

        memset(buffer, 0, sizeof(buffer));
        bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            closesocket(clientSock);
            return;
        }
        password = buffer;
        cout << "[客户端密码] " << password << endl;

        FileManager fm("HAODB");
        LoginManager lm(fm, "HAODB");

        if (lm.loginUser(account, password)) {
            string welcome = "欢迎您 " + current_username;
            sendWithEnd(clientSock, welcome);
            break;
        }
        else {
            sendWithEnd(clientSock, "账号或密码错误，请重试");
        }
    }

    // SQL 循环
    while (true) {
        sendWithEnd(clientSock, "请输入 SQL 语句:");

        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            break;
        }

        string sql = buffer;
        if (sql == "exit" || sql == "quit") {
            sendWithEnd(clientSock, "再见!");
            break;
        }

int generateDBFile();

int main()
{

    std::cout << "Hello World!\n";
    string sql;
    while (true)
    {
        cout << "请输入 SQL 语句: ";
        getline(cin, sql);
        cout << sql << endl;

        if (sql == "exit;") break;
        if (sql == "gen;") {
            generateDBFile();
            continue;
        }
        if (sql.empty()) {
            continue;
        }

        vector<Quadruple> quadruple = sql_compiler(sql);

        string response = "SQL 已解析，共 " + to_string(quadruple.size()) + " 条四元式\n";
        for (auto& q : quadruple) {
            response += "(" + q.op + ", " + q.arg1 + ", " + q.arg2 + ", " + q.result + ")\n";
        }
        sendWithEnd(clientSock, response);
    }

        // 创建 DiskManager
        DiskManager dm("database.db");
        // 创建 BufferPoolManager
        // 缓冲池大小 10
        BufferPoolManager bpm(10, &dm);

// 启动服务器
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

    cout << "服务器已启动，端口 8080，等待客户端连接..." << endl;

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

vector<Quadruple> sql_compiler(string sql) {
    Lexer lexer(sql);
    SQLParser sqlParser;
    cout << "开始词法分析" << endl;
    vector<Token> tokens = lexer.analyze();
    for (auto& t : tokens) {
        cout << left << setw(10) << t.type
            << setw(15) << t.value
            << setw(10) << t.line
            << setw(10) << t.column << endl;
    }

    cout << "开始语法分析" << endl;
    sqlParser.start_parser(tokens);
    SemanticAnalyzer analyzer;
    try {
        auto quads = analyzer.analyze(tokens);

        cout << "四元式结果：" << endl;
        for (auto& q : quads) {
            cout << "(" << q.op << ", " << q.arg1 << ", "
                << q.arg2 << ", " << q.result << ")" << endl;
        }

        return quads;
    }
    catch (const SemanticError& e) {
        cerr << "语义分析错误：" << e.what() << endl;
        return {};
    }
}


/*int main()
{
    std::cout << "Hello World!\n";
    string sql;
    string account, password;
    while (true)
    {
        cout << "请输入账号" << endl;
        cin >> account;
        cout << "请输入密码" << endl;
        cin >> password;
        FileManager fm("HAODB");
        LoginManager lm(fm, "HAODB");
        if (lm.loginUser(account, password)) {
            cout << "欢迎您" << current_username << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }
    

    while (true)
    {
        
        cout << "请输入 SQL 语句: ";
        getline(cin, sql);
        cout << sql << endl;
        vector<Quadruple> quadruple = sql_compiler(sql);

        /*
        // 创建 DiskManager
        DiskManager dm("database.db");
        // 创建 BufferPoolManager
        // 缓冲池大小 10
        BufferPoolManager bpm(10, &dm);

        // 构建并执行计划
        vector<string> columns;
        Operator* root = buildPlan(quadruple, columns, &bpm);
        vector<Row> result = root->execute();

            if (!result.empty()) {
                cout << "主函数打印" << endl;
                // 输出结果
                cout << endl;
                for (auto& row : result) {
                    for (auto& col : columns) {
                        cout << row.at(col) << "\t|";
                    }
                    cout << endl;
                }
            }
        }
   }
}*/



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
    DiskManager dm("database.db");
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

//int main() {
//    int pageId = 0;
//
//    DiskManager dm("database.db");
//    BufferPoolManager bpm(3, &dm);
//    Page* p = bpm.fetchPage(pageId);
//
//    int slotId = p->insertRecord("AAAAAAAAA", 10);
//    cout << slotId << endl;
//
//    dm.writePage(pageId, *p);
//}