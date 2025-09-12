#include <iostream>
#include <vector>
#include "catalog_manager.h"
#include "database_registry_manager.h"
using namespace std;

void test()
{
    // 初始化
    CatalogManager catalog("HAODB2");
    catalog.Initialize();
    /*
    std::vector<DatabaseInfo> listDatabaseInfo = catalog.ListDatabases();
    DatabaseRegistryManager registry_mgr("HAODB2/system/databases.registry");
    registry_mgr.LoadRegistry();
    registry_mgr.ListDatabases();*/
    catalog.ShowSystemInfo();
    catalog.ShowDatabaseList();

    // 创建数据库
    std::cout << endl << "创建数据库Students" << endl;
    catalog.CreateDatabase("Students", "admin");
    catalog.ShowDatabaseList();
    catalog.ShowTableList("Students");
    /*registry_mgr.LoadRegistry();
    registry_mgr.ListDatabases();*/

    // 创建表
    std::cout << endl << "创建数据表student_info" << endl;
    std::vector<std::vector<std::string>> columns = {
        {"id", "INT", "4"},
        {"name", "VARCHAR", "50"},
        {"age", "INT", "4"}
    };
    catalog.CreateTable("Students", "student_info", columns, 0);
    catalog.ShowTableList("Students");
    catalog.ShowTableStructure("Students", "student_info");

    // 设置主键
    catalog.SetPrimaryKey("Students", "student_info", "id", true);
    std::cout << endl << "设置主键后：" << endl;
    catalog.ShowTableStructure("Students", "student_info");
    catalog.SetPrimaryKey("Students", "student_info", "id", false);
    std::cout << endl << "取消主键后：" << endl;
    catalog.ShowTableStructure("Students", "student_info");

    // 创建索引
    catalog.CreateIndex("Students", "student_info", "idx_name", { "name" });

    std::cout << endl << "设置索引后：" << endl;
    catalog.ShowIndexList("Students");

    // 更新统计信息
    catalog.UpdateReadCount("Students", 1);
    catalog.UpdateCacheHits("Students", 1);
    catalog.ShowStatistics("Students");
}