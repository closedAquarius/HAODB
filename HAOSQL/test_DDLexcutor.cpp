#include "executor.h"
#include <iostream>

using namespace std;

/*int testDDL() {
    setDBName("TestDB");

    // 创建模拟的组件
    CatalogManager catalog("TEST_HAODB");
    // 假设有BufferPoolManager（DDL实际上没有使用）
    BufferPoolManager* bmp = nullptr;

    catalog.Initialize();
    catalog.CreateDatabase("TestDB", "admin");

    cout << "=== DDL Executor Usage Examples ===" << endl << endl;

    // ========== 示例1: 创建表 ==========
    cout << "1. CREATE TABLE Example:" << endl;
    vector<Quadruple> create_table_quads = {
        {"COLUMN_NAME", "id,name,age,phone", "", "T1"},
        {"COLUMN_TYPE", "VARCHAR,VARCHAR,INT,VARCHAR", "10,10,-1,15", "T2"},
        {"COLUMNS", "T1", "T2", "T3"},
        {"CREATE_TABLE", "students", "T3", "T4"},
        {"PRIMARY", "students", "id,name", "T5"},
        {"NOT_NULL", "students", "name,age", "T6"},
        {"UNIQUE", "students", "id,phone", "T7"},
        {"RESULT", "", "", ""}
    };

    vector<string> columns;
    try {
        Operator* plan = buildPlan(create_table_quads, columns, bmp, &catalog);

        if (plan) {
            std::cout << "获取算子树" << endl;
            plan->execute();
            delete plan;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << endl;

    catalog.ShowTableList("TestDB");

    // ========== 示例2: 删除表 ==========
    cout << "2. DROP TABLE Example:" << endl;
    vector<Quadruple> drop_table_quads = {
        {"CHECK_TABLE", "student", "", "T1"},
        {"DROP_TABLE", "student", "", "T2"},
        {"RESULT", "", "", ""}
    };

    try {
        Operator* plan = buildPlan(drop_table_quads, columns, bmp, &catalog);
        if (plan) {
            plan->execute();
            delete plan;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << endl;

    // ========== 示例3: 添加列 ==========
    cout << "3. ADD COLUMN Example:" << endl;
    vector<Quadruple> add_column_quads = {
        {"COLUMN_NAME", "sex", "", "T1"},
        {"COLUMN_TYPE", "VARCHAR", "5", "T2"},
        {"COLUMNS", "T1", "T2", "T3"},
        {"ADD_COLUMN", "students", "T3", "T4"},
        {"NOT_NULL", "students", "sex", "T5"},
        {"RESULT", "", "", ""}
    };

    try {
        Operator* plan = buildPlan(add_column_quads, columns, bmp, &catalog);
        if (plan) {
            plan->execute();
            delete plan;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << endl;

    // ========== 示例4: 删除列 ==========
    cout << "4. DROP COLUMN Example:" << endl;
    vector<Quadruple> drop_column_quads = {
        {"DROP_COLUMN", "students", "sex", "T1"},
        {"RESULT", "", "", ""}
    };

    try {
        Operator* plan = buildPlan(drop_column_quads, columns, bmp, &catalog);
        if (plan) {
            plan->execute();
            delete plan;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << endl;

    cout << endl << "=== All DDL operations completed ===" << endl;

    return 0;
}*/