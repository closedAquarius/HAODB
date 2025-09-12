#pragma once
#include "catalog_manager.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <iomanip>
#include <chrono>

class CatalogManagerTest {
private:
    CatalogManager* catalog;
    int passed_tests;
    int total_tests;

    // 测试辅助方法
    void PrintTestHeader(const std::string& test_name);
    void PrintTestResult(const std::string& test_name, bool result);
    void AssertTrue(bool condition, const std::string& message);
    void AssertFalse(bool condition, const std::string& message);
    void AssertEqual(const std::string& expected, const std::string& actual, const std::string& message);

    // 具体测试方法
    void TestInitialization();
    void TestDatabaseManagement();
    void TestTableManagement();
    void TestIndexManagement();
    void TestStatisticsManagement();
    void TestConfigurationManagement();
    void TestDisplayFunctions();
    void TestErrorHandling();
    void TestCleanup();

public:
    CatalogManagerTest();
    ~CatalogManagerTest();

    // 运行所有测试
    bool RunAllTests();

    // 单独测试组
    bool TestBasicOperations();
    bool TestAdvancedOperations();
    bool TestPerformance();

    // 测试报告
    void PrintSummary();
};

// ========================= 实现部分 =========================

CatalogManagerTest::CatalogManagerTest() : catalog(nullptr), passed_tests(0), total_tests(0) {
    std::cout << "=== HAODB CatalogManager 测试套件 ===" << std::endl;
    std::cout << "初始化测试环境..." << std::endl;
}

CatalogManagerTest::~CatalogManagerTest() {
    if (catalog) {
        delete catalog;
    }
}

void CatalogManagerTest::PrintTestHeader(const std::string& test_name) {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "测试: " << test_name << std::endl;
    std::cout << std::string(60, '-') << std::endl;
}

void CatalogManagerTest::PrintTestResult(const std::string& test_name, bool result) {
    total_tests++;
    if (result) {
        passed_tests++;
        std::cout << "[PASS] " << test_name << std::endl;
    }
    else {
        std::cout << "[FAIL] " << test_name << std::endl;
    }
}

void CatalogManagerTest::AssertTrue(bool condition, const std::string& message) {
    total_tests++;
    if (condition) {
        passed_tests++;
        std::cout << "[PASS] " << message << std::endl;
    }
    else {
        std::cout << "[FAIL] " << message << std::endl;
    }
}

void CatalogManagerTest::AssertFalse(bool condition, const std::string& message) {
    AssertTrue(!condition, message);
}

void CatalogManagerTest::AssertEqual(const std::string& expected, const std::string& actual, const std::string& message) {
    total_tests++;
    if (expected == actual) {
        passed_tests++;
        std::cout << "[PASS] " << message << std::endl;
    }
    else {
        std::cout << "[FAIL] " << message << " (期望: " << expected << ", 实际: " << actual << ")" << std::endl;
    }
}

bool CatalogManagerTest::RunAllTests() {
    try {
        TestInitialization();
        TestDatabaseManagement();
        TestTableManagement();
        TestIndexManagement();
        TestStatisticsManagement();
        TestConfigurationManagement();
        TestDisplayFunctions();
        TestErrorHandling();
        TestCleanup();

        PrintSummary();
        return passed_tests == total_tests;
    }
    catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return false;
    }
}

void CatalogManagerTest::TestInitialization() {
    PrintTestHeader("初始化和基础功能测试");

    // 测试1: 创建CatalogManager实例
    catalog = new CatalogManager("TEST_HAODB");
    AssertTrue(catalog != nullptr, "创建CatalogManager实例");

    // 测试2: 初始化
    bool init_result = catalog->Initialize();
    AssertTrue(init_result, "CatalogManager初始化");

    if (!init_result) {
        std::cerr << "初始化失败，无法继续测试" << std::endl;
        return;
    }
}

void CatalogManagerTest::TestDatabaseManagement() {
    PrintTestHeader("数据库管理功能测试");

    // 测试1: 创建数据库
    bool create_db1 = catalog->CreateDatabase("TestDB1", "user1");
    AssertTrue(create_db1, "创建数据库TestDB1");

    bool create_db2 = catalog->CreateDatabase("TestDB2", "user2");
    AssertTrue(create_db2, "创建数据库TestDB2");

    bool create_db3 = catalog->CreateDatabase("TestDB3", "user1");
    AssertTrue(create_db3, "创建数据库TestDB3");

    // 测试2: 检查数据库是否存在
    bool exists1 = catalog->DatabaseExists("TestDB1");
    AssertTrue(exists1, "检查TestDB1是否存在");

    bool not_exists = catalog->DatabaseExists("NonExistentDB");
    AssertFalse(not_exists, "检查不存在的数据库");

    // 测试3: 重复创建数据库（应该失败）
    bool duplicate_create = catalog->CreateDatabase("TestDB1", "user1");
    AssertFalse(duplicate_create, "重复创建数据库应该失败");

    // 测试4: 获取所有数据库列表
    auto all_databases = catalog->ListDatabases();
    AssertTrue(all_databases.size() >= 3, "获取所有数据库列表");

    // 测试5: 按用户过滤数据库列表
    auto user1_databases = catalog->ListDatabases("user1");
    AssertTrue(user1_databases.size() == 2, "按用户过滤数据库列表");

    // 测试6: 获取单个数据库信息
    auto db_info = catalog->GetDatabaseInfo("TestDB1");
    AssertEqual("TestDB1", db_info.db_name, "获取数据库信息");
    AssertEqual("user1", db_info.owner, "验证数据库所有者");

    // 测试7: 无效数据库名称
    bool invalid_name = catalog->CreateDatabase("", "user1");
    AssertFalse(invalid_name, "空数据库名应该失败");

    bool invalid_chars = catalog->CreateDatabase("test@db", "user1");
    AssertFalse(invalid_chars, "包含非法字符的数据库名应该失败");
}

void CatalogManagerTest::TestTableManagement() {
    PrintTestHeader("数据表管理功能测试");

    // 测试1: 创建表
    std::vector<std::vector<std::string>> student_columns = {
        {"student_id", "INT", "4"},
        {"name", "VARCHAR", "50"},
        {"age", "INT", "4"},
        {"email", "VARCHAR", "100"},
        {"gpa", "DECIMAL", "8"}
    };

    bool create_table = catalog->CreateTable("TestDB1", "students", student_columns);
    AssertTrue(create_table, "创建students表");

    // 测试2: 创建另一个表
    std::vector<std::vector<std::string>> course_columns = {
        {"course_id", "INT", "4"},
        {"course_name", "VARCHAR", "100"},
        {"credits", "INT", "4"},
        {"instructor", "VARCHAR", "50"}
    };

    bool create_course_table = catalog->CreateTable("TestDB1", "courses", course_columns);
    AssertTrue(create_course_table, "创建courses表");

    // 测试3: 检查表是否存在
    bool table_exists = catalog->TableExists("TestDB1", "students");
    AssertTrue(table_exists, "检查students表是否存在");

    bool table_not_exists = catalog->TableExists("TestDB1", "nonexistent");
    AssertFalse(table_not_exists, "检查不存在的表");

    // 测试4: 重复创建表（应该失败）
    bool duplicate_table = catalog->CreateTable("TestDB1", "students", student_columns);
    AssertFalse(duplicate_table, "重复创建表应该失败");

    // 测试5: 在不存在的数据库中创建表（应该失败）
    bool invalid_db_table = catalog->CreateTable("NonExistentDB", "test", student_columns);
    AssertFalse(invalid_db_table, "在不存在的数据库中创建表应该失败");

    // 测试6: 设置主键
    bool set_pk = catalog->SetPrimaryKey("TestDB1", "students", "student_id", true);
    AssertTrue(set_pk, "设置student_id为主键");

    // 测试7: 设置唯一约束
    bool set_unique = catalog->SetUnique("TestDB1", "students", "email", true);
    AssertTrue(set_unique, "设置email为唯一");

    // 测试8: 设置可空属性
    bool set_nullable = catalog->SetNullable("TestDB1", "students", "gpa", true);
    AssertTrue(set_nullable, "设置gpa可空");

    // 测试9: 尝试将主键设为可空（应该失败）
    bool pk_nullable = catalog->SetNullable("TestDB1", "students", "student_id", true);
    AssertFalse(pk_nullable, "主键不能设为可空");

    // 测试10: 添加新列
    std::vector<std::vector<std::string>> new_columns = {
        {"phone", "VARCHAR", "20"},
        {"address", "VARCHAR", "200"}
    };

    bool add_columns = catalog->AddColumns("TestDB1", "students", new_columns);
    AssertTrue(add_columns, "添加新列");

    // 测试11: 获取表信息
    auto table_info = catalog->GetTableInfo("TestDB1", "students");
    AssertEqual("students", table_info.table_name, "获取表名");
    AssertTrue(table_info.column_count >= 7, "检查列数");
    for (int i = 0; i < table_info.columns.size(); i++)
    {
        std::cout << "列" << i + 1 << "最大长度" << table_info.columns[i].max_length << std::endl;
        std::cout << "列" << i + 1 << "偏移量" << table_info.columns[i].column_offset << std::endl;
    }

    // 测试12: 获取表列表
    auto tables = catalog->ListTables("TestDB1");
    AssertTrue(tables.size() >= 2, "获取表列表");
    for (int i = 0; i < tables.size(); i++)
    {
        std::cout << "表" << i + 1 << "偏移量" << tables[i].data_file_offset << std::endl;
    }

    // 测试13: 更新行数
    bool update_rows = catalog->UpdateTableRowCount("TestDB1", "students", 1000);
    AssertTrue(update_rows, "更新表行数");

    // 测试14: 删除列
    std::vector<std::string> columns_to_drop = { "phone" };
    bool drop_columns = catalog->DropColumns("TestDB1", "students", columns_to_drop);
    AssertTrue(drop_columns, "删除列");

    // 测试15: 无效的表操作
    bool invalid_column = catalog->SetPrimaryKey("TestDB1", "students", "nonexistent_column", true);
    AssertFalse(invalid_column, "对不存在的列设置主键应该失败");

    // 测试16: 创建带有无效列定义的表
    std::vector<std::vector<std::string>> invalid_columns = {
        {"id"} // 缺少类型和长度
    };
    bool invalid_table = catalog->CreateTable("TestDB1", "invalid_table", invalid_columns);
    AssertFalse(invalid_table, "无效的列定义应该失败");
}

void CatalogManagerTest::TestIndexManagement() {
    PrintTestHeader("索引管理功能测试");

    // 测试1: 创建单列索引
    std::vector<std::string> name_columns = { "name" };
    bool create_name_index = catalog->CreateIndex("TestDB1", "students", "idx_student_name", name_columns);
    AssertTrue(create_name_index, "创建姓名索引");

    // 测试2: 创建唯一索引
    std::vector<std::string> email_columns = { "email" };
    bool create_unique_index = catalog->CreateIndex("TestDB1", "students", "idx_student_email", email_columns, true);
    AssertTrue(create_unique_index, "创建唯一邮箱索引");

    // 测试3: 创建复合索引
    std::vector<std::string> composite_columns = { "age", "gpa" };
    bool create_composite_index = catalog->CreateIndex("TestDB1", "students", "idx_age_gpa", composite_columns);
    AssertTrue(create_composite_index, "创建复合索引");

    // 测试4: 在courses表上创建索引
    std::vector<std::string> course_columns = { "course_name" };
    bool create_course_index = catalog->CreateIndex("TestDB1", "courses", "idx_course_name", course_columns);
    AssertTrue(create_course_index, "创建课程名称索引");

    // 测试5: 检查索引是否存在
    bool index_exists = catalog->IndexExists("TestDB1", "idx_student_name");
    AssertTrue(index_exists, "检查索引是否存在");

    bool index_not_exists = catalog->IndexExists("TestDB1", "nonexistent_index");
    AssertFalse(index_not_exists, "检查不存在的索引");

    // 测试6: 重复创建索引（应该失败）
    bool duplicate_index = catalog->CreateIndex("TestDB1", "students", "idx_student_name", name_columns);
    AssertFalse(duplicate_index, "重复创建索引应该失败");

    // 测试7: 在不存在的列上创建索引（应该失败）
    std::vector<std::string> invalid_columns = { "nonexistent_column" };
    bool invalid_index = catalog->CreateIndex("TestDB1", "students", "idx_invalid", invalid_columns);
    AssertFalse(invalid_index, "在不存在的列上创建索引应该失败");

    // 测试8: 在不存在的表上创建索引（应该失败）
    bool invalid_table_index = catalog->CreateIndex("TestDB1", "nonexistent_table", "idx_test", name_columns);
    AssertFalse(invalid_table_index, "在不存在的表上创建索引应该失败");

    // 测试9: 获取所有索引列表
    auto all_indexes = catalog->ListIndexes("TestDB1");
    AssertTrue(all_indexes.size() >= 4, "获取所有索引列表");

    // 测试10: 获取特定表的索引
    auto student_indexes = catalog->ListIndexes("TestDB1", "students");
    AssertTrue(student_indexes.size() >= 3, "获取students表的索引");

    // 新增测试：获取制定列的索引
    auto certain_indexes = catalog->FindIndexesWithColumns("TestDB1", "students", {"gpa", "age"});
    AssertTrue(certain_indexes.size() >= 1, "获取特定列的索引");

    // 测试11: 删除索引
    bool drop_index = catalog->DropIndex("TestDB1", "idx_age_gpa");
    AssertTrue(drop_index, "删除复合索引");

    // 测试12: 删除不存在的索引（应该失败）
    bool drop_nonexistent = catalog->DropIndex("TestDB1", "nonexistent_index");
    AssertFalse(drop_nonexistent, "删除不存在的索引应该失败");

    // 测试13: 验证索引删除
    bool deleted_index_exists = catalog->IndexExists("TestDB1", "idx_age_gpa");
    AssertFalse(deleted_index_exists, "验证索引已删除");
}

void CatalogManagerTest::TestStatisticsManagement() {
    PrintTestHeader("统计信息管理功能测试");

    // 测试1: 更新总页面数
    bool update_total_pages = catalog->UpdateTotalPages("TestDB1", 100);
    AssertTrue(update_total_pages, "更新总页面数");

    // 测试2: 更新空闲页面数
    bool update_free_pages = catalog->UpdateFreePages("TestDB1", 20);
    AssertTrue(update_free_pages, "更新空闲页面数");

    // 测试3: 更新数据库大小
    bool update_size = catalog->UpdateDatabaseSize("TestDB1", 1048576); // 1MB
    AssertTrue(update_size, "更新数据库大小");

    // 测试4: 更新事务计数
    bool update_transactions = catalog->UpdateTransactionCount("TestDB1", 50);
    AssertTrue(update_transactions, "更新事务计数");

    // 测试5: 更新读取计数
    bool update_reads = catalog->UpdateReadCount("TestDB1", 1000);
    AssertTrue(update_reads, "更新读取计数");

    // 测试6: 更新写入计数
    bool update_writes = catalog->UpdateWriteCount("TestDB1", 200);
    AssertTrue(update_writes, "更新写入计数");

    // 测试7: 更新缓存命中
    bool update_cache_hits = catalog->UpdateCacheHits("TestDB1", 800);
    AssertTrue(update_cache_hits, "更新缓存命中");

    // 测试8: 更新缓存未命中
    bool update_cache_misses = catalog->UpdateCacheMisses("TestDB1", 200);
    AssertTrue(update_cache_misses, "更新缓存未命中");

    // 测试9: 更新活跃连接数
    bool update_connections = catalog->UpdateActiveConnections("TestDB1", 15);
    AssertTrue(update_connections, "更新活跃连接数");

    // 测试10: 获取统计信息
    auto stats = catalog->GetDatabaseStatistics("TestDB1");
    AssertTrue(stats.total_pages >= 100, "验证总页面数");
    AssertTrue(stats.free_pages >= 20, "验证空闲页面数");
    AssertTrue(stats.total_size >= 1048576, "验证数据库大小");
    AssertTrue(stats.transaction_count >= 50, "验证事务计数");
    AssertTrue(stats.total_reads >= 1000, "验证读取计数");
    AssertTrue(stats.total_writes >= 200, "验证写入计数");
    AssertTrue(stats.cache_hits >= 800, "验证缓存命中");
    AssertTrue(stats.cache_misses >= 200, "验证缓存未命中");
    AssertTrue(stats.active_connections == 15, "验证活跃连接数");
    AssertTrue(stats.cache_hit_ratio > 0, "验证缓存命中率计算");
    AssertTrue(stats.storage_utilization > 0, "验证存储利用率计算");

    // 测试11: 在不存在的数据库上更新统计信息（应该失败）
    bool invalid_stats = catalog->UpdateTotalPages("NonExistentDB", 10);
    AssertFalse(invalid_stats, "在不存在的数据库上更新统计信息应该失败");

    // 测试12: 连续更新测试
    bool multiple_updates = true;
    for (int i = 0; i < 10; i++) {
        if (!catalog->UpdateReadCount("TestDB1", 10)) {
            multiple_updates = false;
            break;
        }
    }
    AssertTrue(multiple_updates, "连续多次更新统计信息");

    // 测试13: 重置统计信息
    bool reset_stats = catalog->ResetStatistics("TestDB1");
    AssertTrue(reset_stats, "重置统计信息");

    // 测试14: 验证重置效果
    auto reset_stats_result = catalog->GetDatabaseStatistics("TestDB1");
    AssertTrue(reset_stats_result.total_reads == 0, "验证读取计数已重置");
    AssertTrue(reset_stats_result.total_writes == 0, "验证写入计数已重置");
    AssertTrue(reset_stats_result.cache_hits == 0, "验证缓存命中已重置");
}

void CatalogManagerTest::TestConfigurationManagement() {
    PrintTestHeader("配置管理功能测试");

    // 测试1: 设置最大数据库数
    bool set_max_db = catalog->SetMaxDatabases(200);
    AssertTrue(set_max_db, "设置最大数据库数");

    // 测试2: 设置最大连接数
    bool set_max_conn = catalog->SetMaxConnections(2000);
    AssertTrue(set_max_conn, "设置最大连接数");

    // 测试3: 设置全局缓冲区大小
    bool set_buffer = catalog->SetGlobalBufferSize(268435456); // 256MB
    AssertTrue(set_buffer, "设置全局缓冲区大小");

    // 测试4: 启用查询缓存
    bool enable_cache = catalog->EnableQueryCache(true);
    AssertTrue(enable_cache, "启用查询缓存");

    // 测试5: 启用身份验证
    bool enable_auth = catalog->EnableAuthentication(true);
    AssertTrue(enable_auth, "启用身份验证");

    // 测试6: 禁用查询缓存
    bool disable_cache = catalog->EnableQueryCache(false);
    AssertTrue(disable_cache, "禁用查询缓存");

    // 测试7: 获取系统配置
    const auto& config = catalog->GetSystemConfig();
    AssertTrue(config.max_databases == 200, "验证最大数据库数配置");
    AssertTrue(config.max_connections_total == 2000, "验证最大连接数配置");
    AssertTrue(config.global_buffer_size == 268435456, "验证缓冲区大小配置");
    AssertTrue(config.enable_query_cache == 0, "验证查询缓存配置");
    AssertTrue(config.enable_authentication == 1, "验证身份验证配置");
}

void CatalogManagerTest::TestDisplayFunctions() {
    PrintTestHeader("显示功能测试");

    std::cout << "\n--- 以下为显示功能测试输出 ---" << std::endl;

    // 测试1: 显示系统信息
    std::cout << "\n1. 系统信息显示:" << std::endl;
    catalog->ShowSystemInfo();
    AssertTrue(true, "系统信息显示");

    // 测试2: 显示数据库列表
    std::cout << "\n2. 数据库列表显示:" << std::endl;
    catalog->ShowDatabaseList();
    AssertTrue(true, "数据库列表显示");

    // 测试3: 显示表列表
    std::cout << "\n3. 表列表显示:" << std::endl;
    catalog->ShowTableList("TestDB1");
    AssertTrue(true, "表列表显示");

    // 测试4: 显示表结构
    std::cout << "\n4. 表结构显示:" << std::endl;
    catalog->ShowTableStructure("TestDB1", "students");
    AssertTrue(true, "表结构显示");

    // 测试5: 显示索引列表
    std::cout << "\n5. 索引列表显示:" << std::endl;
    catalog->ShowIndexList("TestDB1");
    AssertTrue(true, "索引列表显示");

    // 测试6: 显示统计信息
    std::cout << "\n6. 统计信息显示:" << std::endl;
    catalog->ShowStatistics("TestDB1");
    AssertTrue(true, "统计信息显示");

    // 测试7: 生成系统报告
    std::cout << "\n7. 系统报告生成:" << std::endl;
    catalog->GenerateSystemReport();
    AssertTrue(true, "系统报告生成");

    // 测试8: 生成数据库报告
    std::cout << "\n8. 数据库报告生成:" << std::endl;
    catalog->GenerateDatabaseReport("TestDB1");
    AssertTrue(true, "数据库报告生成");

    std::cout << "\n--- 显示功能测试输出结束 ---" << std::endl;
}

void CatalogManagerTest::TestErrorHandling() {
    PrintTestHeader("错误处理测试");

    // 测试1: 无效的数据库名
    bool invalid_db_name1 = catalog->CreateDatabase("", "user1");
    AssertFalse(invalid_db_name1, "空数据库名");

    bool invalid_db_name2 = catalog->CreateDatabase("test-db", "user1");
    AssertFalse(invalid_db_name2, "包含非法字符的数据库名");

    std::string long_name(100, 'a');
    bool invalid_db_name3 = catalog->CreateDatabase(long_name, "user1");
    AssertFalse(invalid_db_name3, "过长的数据库名");

    // 测试2: 在不存在的数据库上执行操作
    std::vector<std::vector<std::string>> test_columns = { {"id", "INT", "4"} };
    bool invalid_db_op = catalog->CreateTable("NonExistentDB", "test", test_columns);
    AssertFalse(invalid_db_op, "在不存在的数据库上创建表");

    // 测试3: 无效的表名
    bool invalid_table_name1 = catalog->CreateTable("TestDB1", "", test_columns);
    AssertFalse(invalid_table_name1, "空表名");

    bool invalid_table_name2 = catalog->CreateTable("TestDB1", "test@table", test_columns);
    AssertFalse(invalid_table_name2, "包含非法字符的表名");

    // 测试4: 无效的列定义
    std::vector<std::vector<std::string>> invalid_columns1 = { {""} };
    bool invalid_column_def1 = catalog->CreateTable("TestDB1", "test_table", invalid_columns1);
    AssertFalse(invalid_column_def1, "无效的列定义");

    std::vector<std::vector<std::string>> invalid_columns2 = {};
    bool invalid_column_def2 = catalog->CreateTable("TestDB1", "test_table2", invalid_columns2);
    AssertFalse(invalid_column_def2, "空的列定义");

    // 测试5: 对不存在的表执行操作
    bool invalid_table_op1 = catalog->SetPrimaryKey("TestDB1", "nonexistent_table", "id", true);
    AssertFalse(invalid_table_op1, "对不存在的表设置主键");

    bool invalid_table_op2 = catalog->DropTable("TestDB1", "nonexistent_table");
    AssertFalse(invalid_table_op2, "删除不存在的表");

    // 测试6: 对不存在的列执行操作
    bool invalid_column_op = catalog->SetPrimaryKey("TestDB1", "students", "nonexistent_column", true);
    AssertFalse(invalid_column_op, "对不存在的列设置主键");

    // 测试7: 违反约束的操作
    // 先设置一个列为主键
    catalog->SetPrimaryKey("TestDB1", "students", "student_id", true);
    // 然后尝试将主键设为可空（应该失败）
    bool constraint_violation = catalog->SetNullable("TestDB1", "students", "student_id", true);
    AssertFalse(constraint_violation, "主键不能设为可空");

    // 测试8: 索引相关错误
    std::vector<std::string> invalid_index_columns = { "nonexistent_column" };
    bool invalid_index = catalog->CreateIndex("TestDB1", "students", "invalid_idx", invalid_index_columns);
    AssertFalse(invalid_index, "在不存在的列上创建索引");

    bool invalid_index_table = catalog->CreateIndex("TestDB1", "nonexistent_table", "test_idx", { "id" });
    AssertFalse(invalid_index_table, "在不存在的表上创建索引");

    // 测试9: 统计信息错误
    bool invalid_stats_db = catalog->UpdateTotalPages("NonExistentDB", 10);
    AssertFalse(invalid_stats_db, "在不存在的数据库上更新统计信息");
}

void CatalogManagerTest::TestCleanup() {
    PrintTestHeader("清理测试");

    // 测试1: 删除创建的表（测试级联删除）
    bool drop_courses = catalog->DropTable("TestDB1", "courses");
    AssertTrue(drop_courses, "删除courses表");

    bool drop_students = catalog->DropTable("TestDB1", "students");
    AssertTrue(drop_students, "删除students表");

    // 测试2: 验证表已删除
    bool students_exists = catalog->TableExists("TestDB1", "students");
    AssertFalse(students_exists, "验证students表已删除");

    bool courses_exists = catalog->TableExists("TestDB1", "courses");
    AssertFalse(courses_exists, "验证courses表已删除");

    // 测试3: 验证索引已级联删除
    bool index_exists = catalog->IndexExists("TestDB1", "idx_student_name");
    AssertFalse(index_exists, "验证索引已级联删除");

    // 测试4: 删除数据库
    bool drop_db1 = catalog->DropDatabase("TestDB1");
    AssertTrue(drop_db1, "删除TestDB1");

    bool drop_db2 = catalog->DropDatabase("TestDB2");
    AssertTrue(drop_db2, "删除TestDB2");

    bool drop_db3 = catalog->DropDatabase("TestDB3");
    AssertTrue(drop_db3, "删除TestDB3");

    // 测试5: 验证数据库已删除
    bool db1_exists = catalog->DatabaseExists("TestDB1");
    AssertFalse(db1_exists, "验证TestDB1已删除");

    // 测试6: 最终关闭
    bool shutdown_result = catalog->Shutdown();
    AssertTrue(shutdown_result, "CatalogManager关闭");
}

bool CatalogManagerTest::TestBasicOperations() {
    PrintTestHeader("基础操作测试套件");

    int initial_passed = passed_tests;
    int initial_total = total_tests;

    TestInitialization();
    TestDatabaseManagement();
    TestTableManagement();

    int basic_passed = passed_tests - initial_passed;
    int basic_total = total_tests - initial_total;

    std::cout << "基础操作测试结果: " << basic_passed << "/" << basic_total << " 通过" << std::endl;
    return basic_passed == basic_total;
}

bool CatalogManagerTest::TestAdvancedOperations() {
    PrintTestHeader("高级操作测试套件");

    int initial_passed = passed_tests;
    int initial_total = total_tests;

    TestIndexManagement();
    TestStatisticsManagement();
    TestConfigurationManagement();

    int advanced_passed = passed_tests - initial_passed;
    int advanced_total = total_tests - initial_total;

    std::cout << "高级操作测试结果: " << advanced_passed << "/" << advanced_total << " 通过" << std::endl;
    return advanced_passed == advanced_total;
}

bool CatalogManagerTest::TestPerformance() {
    PrintTestHeader("性能测试");

    auto start_time = std::chrono::high_resolution_clock::now();

    // 批量创建数据库
    for (int i = 0; i < 10; i++) {
        std::string db_name = "PerfDB" + std::to_string(i);
        catalog->CreateDatabase(db_name, "perf_user");
    }

    // 批量创建表
    std::vector<std::vector<std::string>> perf_columns = {
        {"id", "INT", "4"},
        {"data", "VARCHAR", "100"}
    };

    for (int i = 0; i < 10; i++) {
        std::string db_name = "PerfDB" + std::to_string(i);
        for (int j = 0; j < 5; j++) {
            std::string table_name = "table" + std::to_string(j);
            catalog->CreateTable(db_name, table_name, perf_columns);
        }
    }

    // 批量创建索引
    std::vector<std::string> index_columns = { "id" };
    for (int i = 0; i < 10; i++) {
        std::string db_name = "PerfDB" + std::to_string(i);
        for (int j = 0; j < 5; j++) {
            std::string table_name = "table" + std::to_string(j);
            std::string index_name = "idx_" + table_name + "_id";
            catalog->CreateIndex(db_name, table_name, index_name, index_columns);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "性能测试完成，耗时: " << duration.count() << " 毫秒" << std::endl;
    std::cout << "创建了 10 个数据库，50 个表，50 个索引" << std::endl;

    // 清理性能测试数据
    for (int i = 0; i < 10; i++) {
        std::string db_name = "PerfDB" + std::to_string(i);
        catalog->DropDatabase(db_name);
    }

    AssertTrue(duration.count() < 10000, "性能测试在10秒内完成");
    return duration.count() < 10000;
}

void CatalogManagerTest::PrintSummary() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                    测试结果汇总" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    std::cout << "总测试数: " << total_tests << std::endl;
    std::cout << "通过测试: " << passed_tests << std::endl;
    std::cout << "失败测试: " << (total_tests - passed_tests) << std::endl;

    double success_rate = total_tests > 0 ? (double)passed_tests / total_tests * 100.0 : 0.0;
    std::cout << "成功率: " << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;

    if (passed_tests == total_tests) {
        std::cout << "\n🎉 所有测试通过！CatalogManager 功能正常。" << std::endl;
    }
    else {
        std::cout << "\n⚠️  有测试失败，请检查相关功能。" << std::endl;
    }

    std::cout << std::string(80, '=') << std::endl;
}

// ========================= 主测试函数 =========================

/*
int main() {
    CatalogManagerTest test;

    bool all_tests_passed = test.RunAllTests();

    std::cout << "\n=== 分组测试 ===" << std::endl;

    // 可以单独运行测试组
    /*
    CatalogManagerTest test2;
    test2.TestBasicOperations();

    CatalogManagerTest test3;
    test3.TestAdvancedOperations();

    CatalogManagerTest test4;
    test4.TestPerformance();
    */
    //return all_tests_passed ? 0 : 1;
//}