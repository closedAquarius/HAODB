#include <iostream>
#include <vector>
#include "catalog_manager.h"
#include "database_registry_manager.h"
using namespace std;

void test()
{
    // ��ʼ��
    CatalogManager catalog("HAODB2");
    catalog.Initialize();
    /*
    std::vector<DatabaseInfo> listDatabaseInfo = catalog.ListDatabases();
    DatabaseRegistryManager registry_mgr("HAODB2/system/databases.registry");
    registry_mgr.LoadRegistry();
    registry_mgr.ListDatabases();*/
    catalog.ShowSystemInfo();
    catalog.ShowDatabaseList();

    // �������ݿ�
    std::cout << endl << "�������ݿ�Students" << endl;
    catalog.CreateDatabase("Students", "admin");
    catalog.ShowDatabaseList();
    catalog.ShowTableList("Students");
    /*registry_mgr.LoadRegistry();
    registry_mgr.ListDatabases();*/

    // ������
    std::cout << endl << "�������ݱ�student_info" << endl;
    std::vector<std::vector<std::string>> columns = {
        {"id", "INT", "4"},
        {"name", "VARCHAR", "50"},
        {"age", "INT", "4"}
    };
    catalog.CreateTable("Students", "student_info", columns, 0);
    catalog.ShowTableList("Students");
    catalog.ShowTableStructure("Students", "student_info");

    // ��������
    catalog.SetPrimaryKey("Students", "student_info", "id", true);
    std::cout << endl << "����������" << endl;
    catalog.ShowTableStructure("Students", "student_info");
    catalog.SetPrimaryKey("Students", "student_info", "id", false);
    std::cout << endl << "ȡ��������" << endl;
    catalog.ShowTableStructure("Students", "student_info");

    // ��������
    catalog.CreateIndex("Students", "student_info", "idx_name", { "name" });

    std::cout << endl << "����������" << endl;
    catalog.ShowIndexList("Students");

    // ����ͳ����Ϣ
    catalog.UpdateReadCount("Students", 1);
    catalog.UpdateCacheHits("Students", 1);
    catalog.ShowStatistics("Students");
}