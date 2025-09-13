// HAOSQL.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "dataType.h"
#include "parser.h"
#include "lexer.h"
#include "semantic.h"
#include "executor.h"
#include "buffer_pool.h"
#include "login.h"

using namespace std;


vector<Quadruple> sql_compiler(string sql);
int main()
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
        }*/
   }
}

vector<Quadruple> sql_compiler(string sql)
{
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

    for (int i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i].type << " " << tokens[i].value << " ";
    }
    cout << "开始语法分析" << endl;
    sqlParser.start_parser(tokens);
    SemanticAnalyzer analyzer;
    try {
        auto quads = analyzer.analyze(tokens);

        std::cout << "四元式结果：" << std::endl;
        for (auto& q : quads) {
            std::cout << "(" << q.op << ", " << q.arg1 << ", "
                << q.arg2 << ", " << q.result << ")" << std::endl;
        }

        return analyzer.analyze(tokens);
    }
    catch (const SemanticError& e) {
        std::cerr << "语义分析错误：" << e.what() << std::endl;
        return {};
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
//int main() {
//    DiskManager dm("database.db");
//    Page p(DATA_PAGE);
//
//    // 假设这些是你的Students表数据
//    vector<string> records = {
//        "id:1,name:Alice,age:23,grade:A",
//        "id:2,name:Bob,age:19,grade:B",
//        "id:3,name:Charlie,age:25,grade:A",
//        "id:4,name:John,age:18,grade:A"
//    };
//
//    // 插入记录到页中
//    for (const auto& rec : records) {
//        p.insertRecord(rec.c_str(), rec.length());
//    }
//
//    // 将页写回磁盘
//    dm.writePage(0, p);
//
//    cout << "Successfully created database.db with Students table data on page 0." << endl;
//
//    return 0;
//}