#include "executor.h"
#include <iostream>

using namespace std;

/*int testDDL() {
    setDBName("TestDB");

    // ����ģ������
    CatalogManager catalog("TEST_HAODB");
    // ������BufferPoolManager��DDLʵ����û��ʹ�ã�
    BufferPoolManager* bmp = nullptr;

    catalog.Initialize();
    catalog.CreateDatabase("TestDB", "admin");

    cout << "=== DDL Executor Usage Examples ===" << endl << endl;

    // ========== ʾ��1: ������ ==========
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
            std::cout << "��ȡ������" << endl;
            plan->execute();
            delete plan;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << endl;

    catalog.ShowTableList("TestDB");

    // ========== ʾ��2: ɾ���� ==========
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

    // ========== ʾ��3: ����� ==========
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

    // ========== ʾ��4: ɾ���� ==========
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