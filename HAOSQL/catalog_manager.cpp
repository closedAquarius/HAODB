#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include "database_registry_manager.h"
#include "system_config_manager.h"
#include "database_metadata_manager.h"

// 模拟数据库操作的辅助函数
void SimulateDatabaseOperations(DatabaseMetadataManager& db_mgr) {
    std::cout << "\n--- 模拟数据库操作 ---" << std::endl;

    // 模拟一些I/O操作
    db_mgr.UpdateIOStats(1000, 500);  // 1000次读取，500次写入
    db_mgr.UpdateCacheStats(800, 700); // 800次命中，700次未命中

    // 模拟事务处理
    for (int i = 0; i < 100; i++) {
        db_mgr.IncrementTransactionCount();
    }

    // 模拟连接变化
    db_mgr.SetActiveConnections(15);

    // 模拟存储变化
    db_mgr.UpdateTotalPages(2048, 512);  // 总页面2048，空闲512
    db_mgr.UpdateDatabaseSize(8388608);  // 8MB大小

    std::cout << "模拟操作完成" << std::endl;
}

// 创建Student表的列定义
std::vector<ColumnMeta> CreateStudentColumns() {
    std::vector<ColumnMeta> columns;

    // 学号列
    ColumnMeta sno_col;
    strcpy(sno_col.column_name, "Sno");
    sno_col.column_id = 1;
    sno_col.data_type = 2;  // VARCHAR
    sno_col.max_length = 12;
    sno_col.is_nullable = 0;
    sno_col.is_primary_key = 1;
    sno_col.is_unique = 1;
    sno_col.column_offset = 0;
    columns.push_back(sno_col);

    // 姓名列
    ColumnMeta sname_col;
    strcpy(sname_col.column_name, "Sname");
    sname_col.column_id = 2;
    sname_col.data_type = 2;  // VARCHAR
    sname_col.max_length = 20;
    sname_col.is_nullable = 0;
    sname_col.column_offset = 12;
    columns.push_back(sname_col);

    // 性别列
    ColumnMeta ssex_col;
    strcpy(ssex_col.column_name, "Ssex");
    ssex_col.column_id = 3;
    ssex_col.data_type = 2;  // VARCHAR
    ssex_col.max_length = 2;
    ssex_col.is_nullable = 1;
    ssex_col.column_offset = 32;
    columns.push_back(ssex_col);

    // 年龄列
    ColumnMeta sage_col;
    strcpy(sage_col.column_name, "Sage");
    sage_col.column_id = 4;
    sage_col.data_type = 1;  // INT
    sage_col.max_length = 4;
    sage_col.is_nullable = 1;
    sage_col.column_offset = 34;
    columns.push_back(sage_col);

    // 所在系列
    ColumnMeta sdept_col;
    strcpy(sdept_col.column_name, "Sdept");
    sdept_col.column_id = 5;
    sdept_col.data_type = 2;  // VARCHAR
    sdept_col.max_length = 20;
    sdept_col.is_nullable = 1;
    sdept_col.column_offset = 38;
    columns.push_back(sdept_col);

    return columns;
}

// 创建Course表的列定义
std::vector<ColumnMeta> CreateCourseColumns() {
    std::vector<ColumnMeta> columns;

    // 课程号列
    ColumnMeta cno_col;
    strcpy(cno_col.column_name, "Cno");
    cno_col.column_id = 1;
    cno_col.data_type = 2;  // VARCHAR
    cno_col.max_length = 10;
    cno_col.is_nullable = 0;
    cno_col.is_primary_key = 1;
    cno_col.is_unique = 1;
    cno_col.column_offset = 0;
    columns.push_back(cno_col);

    // 课程名列
    ColumnMeta cname_col;
    strcpy(cname_col.column_name, "Cname");
    cname_col.column_id = 2;
    cname_col.data_type = 2;  // VARCHAR
    cname_col.max_length = 50;
    cname_col.is_nullable = 0;
    cname_col.column_offset = 10;
    columns.push_back(cname_col);

    // 先行课列
    ColumnMeta cpno_col;
    strcpy(cpno_col.column_name, "Cpno");
    cpno_col.column_id = 3;
    cpno_col.data_type = 2;  // VARCHAR
    cpno_col.max_length = 10;
    cpno_col.is_nullable = 1;
    cpno_col.column_offset = 60;
    columns.push_back(cpno_col);

    // 学分列
    ColumnMeta ccredit_col;
    strcpy(ccredit_col.column_name, "Ccredit");
    ccredit_col.column_id = 4;
    ccredit_col.data_type = 1;  // INT
    ccredit_col.max_length = 4;
    ccredit_col.is_nullable = 0;
    strcpy(ccredit_col.default_value, "0");
    ccredit_col.column_offset = 70;
    columns.push_back(ccredit_col);

    return columns;
}

// 创建SC表的列定义
std::vector<ColumnMeta> CreateSCColumns() {
    std::vector<ColumnMeta> columns;

    // 学号列
    ColumnMeta sno_col;
    strcpy(sno_col.column_name, "Sno");
    sno_col.column_id = 1;
    sno_col.data_type = 2;  // VARCHAR
    sno_col.max_length = 12;
    sno_col.is_nullable = 0;
    sno_col.is_primary_key = 1;  // 复合主键
    sno_col.column_offset = 0;
    columns.push_back(sno_col);

    // 课程号列
    ColumnMeta cno_col;
    strcpy(cno_col.column_name, "Cno");
    cno_col.column_id = 2;
    cno_col.data_type = 2;  // VARCHAR
    cno_col.max_length = 10;
    cno_col.is_nullable = 0;
    cno_col.is_primary_key = 1;  // 复合主键
    cno_col.column_offset = 12;
    columns.push_back(cno_col);

    // 成绩列
    ColumnMeta grade_col;
    strcpy(grade_col.column_name, "Grade");
    grade_col.column_id = 3;
    grade_col.data_type = 1;  // INT
    grade_col.max_length = 4;
    grade_col.is_nullable = 1;
    grade_col.column_offset = 22;
    columns.push_back(grade_col);

    return columns;
}

/*
int main() {
    std::cout << "=== HAODB 完整元数据管理系统演示 ===" << std::endl;
    std::cout << "版本: 1.0 (包含配置管理)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 1. 系统配置管理
    std::cout << "\n1. 系统配置管理" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    SystemConfigManager config_mgr("HAODB\\system\\system_config.hao");
    config_mgr.LoadConfig();

    // 调整系统配置
    config_mgr.SetMaxDatabases(20);
    config_mgr.SetMaxConnections(200);
    config_mgr.SetGlobalBufferSize(67108864);  // 64MB
    config_mgr.EnableQueryCache(true);
    config_mgr.EnableAuthentication(true);
    config_mgr.SetSystemPaths("HAODB\\system\\logs\\",
        "HAODB\\temp\\",
        "HAODB\\backup\\");

    config_mgr.ShowConfig();
    config_mgr.SaveConfig();

    // 2. 数据库注册表管理
    std::cout << "\n2. 数据库注册表管理" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    DatabaseRegistryManager registry_mgr("HAODB/system/databases.registry");
    registry_mgr.LoadRegistry();

    // 注册Students数据库
    registry_mgr.RegisterDatabase("Students", "HAODB\\Students\\", 1, "admin");
    registry_mgr.UpdateDatabaseStatus("Students", 1);  // 设置为在线
    registry_mgr.ListDatabases();

    // 3. 创建和管理Students数据库
    std::cout << "\n3. Students数据库管理" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    DatabaseMetadataManager students_db("HAODB/Students/meta_data.hao");

    // 尝试加载，如果失败则创建新数据库
    if (!students_db.LoadMetadata()) {
        std::cout << "创建新的Students数据库..." << std::endl;
        students_db.CreateNewDatabase("Students", 1);

        // 创建表结构
        std::cout << "创建表结构..." << std::endl;
        auto student_columns = CreateStudentColumns();
        students_db.CreateTable("Student", 1, student_columns, 8192);

        auto course_columns = CreateCourseColumns();
        students_db.CreateTable("Course", 2, course_columns, 1048576);

        auto sc_columns = CreateSCColumns();
        students_db.CreateTable("SC", 3, sc_columns, 1572864);

        // 创建索引
        std::cout << "创建索引..." << std::endl;
        students_db.CreateIndex("pk_student_sno", 1, 1, { 1 }, true, 100);
        students_db.CreateIndex("idx_student_sname", 2, 1, { 2 }, false, 200);
        students_db.CreateIndex("pk_course_cno", 3, 2, { 1 }, true, 300);
        students_db.CreateIndex("pk_sc_sno_cno", 4, 3, { 1, 2 }, true, 400);
        students_db.CreateIndex("idx_sc_sno", 5, 3, { 1 }, false, 500);

        students_db.SaveMetadata();
    }

    // 4. 存储配置管理演示
    std::cout << "\n4. 存储配置管理" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 获取当前存储配置
    const StorageConfig& current_storage = students_db.GetStorageConfig();
    StorageConfigManager storage_mgr(const_cast<StorageConfig&>(current_storage));

    // 调整存储配置
    students_db.SetBufferPoolSize(67108864);      // 64MB缓冲池
    students_db.SetMaxConnections(100);           // 最大100连接
    students_db.SetLockTimeout(60);               // 60秒锁超时
    students_db.EnableAutoCheckpoint(true);       // 启用自动检查点
    students_db.SetPageCacheSize(33554432);       // 32MB页面缓存
    students_db.EnableCompression(false);         // 禁用压缩

    // 设置文件路径
    students_db.SetDataFilePath("HAODB\\Students\\data\\students.dat");
    students_db.SetLogFilePath("HAODB\\Students\\logs\\students.log");
    students_db.SetIndexFilePath("HAODB\\Students\\index\\students.idx");

    students_db.ShowStorageConfig();

    // 5. 日志配置管理演示
    std::cout << "\n5. 日志配置管理" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 调整日志配置
    students_db.SetLogLevel(2);                   // INFO级别
    students_db.SetLogFileSize(100);              // 100MB日志文件
    students_db.SetLogFileCount(10);              // 保留10个文件
    students_db.EnableWAL(true);                  // 启用WAL
    students_db.SetWALBufferSize(16777216);       // 16MB WAL缓冲区
    students_db.SetSyncMode(1);                   // 同步模式
    students_db.EnableLogRotation(true);          // 启用日志轮转

    students_db.ShowLogConfig();

    // 6. 数据操作和统计信息演示
    std::cout << "\n6. 数据操作和统计信息" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 更新表数据
    students_db.UpdateTableRowCount("Student", 1500);
    students_db.UpdateTableRowCount("Course", 300);
    students_db.UpdateTableRowCount("SC", 8000);

    // 模拟数据库操作
    SimulateDatabaseOperations(students_db);

    // 设置备份时间
    students_db.SetLastBackupTime(static_cast<uint64_t>(time(nullptr)));

    students_db.ShowDatabaseStats();

    // 7. 使用辅助管理类演示
    std::cout << "\n7. 辅助管理类演示" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 日志配置管理器
    const LogConfig& current_log = students_db.GetLogConfig();
    LogConfigManager log_mgr(const_cast<LogConfig&>(current_log));

    std::cout << "当前日志级别: " << log_mgr.GetLogLevelString() << std::endl;
    std::cout << "当前同步模式: " << log_mgr.GetSyncModeString() << std::endl;

    // 统计信息管理器
    const DatabaseStats& current_stats = students_db.GetDatabaseStats();
    DatabaseStatsManager stats_mgr(const_cast<DatabaseStats&>(current_stats));

    std::cout << "缓存命中率: " << std::fixed << std::setprecision(2)
        << stats_mgr.GetCacheHitRatio() << "%" << std::endl;
    std::cout << "存储利用率: " << std::fixed << std::setprecision(2)
        << stats_mgr.GetStorageUtilization() << "%" << std::endl;

    // 8. 性能分析报告
    std::cout << "\n8. 性能分析报告" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.GeneratePerformanceReport();

    // 9. 显示完整数据库信息
    std::cout << "\n9. 完整数据库信息" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.ShowCompleteInfo();

    // 10. 保存所有更改
    std::cout << "\n10. 保存配置和元数据" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.SaveMetadata();

    // 11. 配置验证演示
    std::cout << "\n11. 配置验证" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 验证存储配置
    if (storage_mgr.ValidateConfig()) {
        std::cout << "存储配置验证通过" << std::endl;
    }
    else {
        std::cout << "存储配置验证失败" << std::endl;
    }

    // 验证日志配置
    /*if (log_mgr.ValidateConfig()) {
        std::cout << "日志配置验证通过" << std::endl;
    }
    else {
        std::cout << "日志配置验证失败" << std::endl;
    }*//*

    // 12. 动态配置调整演示
    std::cout << "\n12. 动态配置调整演示" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // 模拟运行时配置调整
    std::cout << "模拟高负载场景，调整配置..." << std::endl;
    students_db.SetBufferPoolSize(134217728);    // 增加到128MB
    students_db.SetMaxConnections(200);          // 增加到200连接
    students_db.SetLogLevel(1);                  // 降低到WARN级别

    // 模拟一段时间后的统计
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    students_db.UpdateIOStats(5000, 2000);       // 更多I/O操作
    students_db.UpdateCacheStats(4500, 500);     // 更好的缓存表现
    students_db.SetActiveConnections(45);        // 更多活跃连接

    std::cout << "高负载后的性能报告:" << std::endl;
    students_db.GeneratePerformanceReport();

    // 13. 最终保存和清理
    std::cout << "\n13. 最终保存" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    if (students_db.SaveMetadata()) {
        std::cout << "所有配置和元数据保存成功" << std::endl;
    }
    else {
        std::cout << "保存失败" << std::endl;
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "完整元数据管理系统演示完成！" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 显示最终统计信息
    std::cout << "\n最终统计信息:" << std::endl;
    std::cout << "- 管理的表数量: " << students_db.GetTableCount() << std::endl;
    std::cout << "- 管理的索引数量: " << students_db.GetIndexCount() << std::endl;
    std::cout << "- 总记录数: " << students_db.GetTotalRowCount() << std::endl;
    std::cout << "- 缓存命中率: " << std::fixed << std::setprecision(2)
        << students_db.GetCacheHitRatio() << "%" << std::endl;
    std::cout << "- 平均事务大小: " << students_db.GetAverageTransactionSize() << " I/O" << std::endl;

    return 0;
}
*/

//==================== 编译脚本 compile_complete.sh ====================
/*
#!/bin/bash
# 完整项目编译脚本

echo "编译HAODB完整元数据管理系统..."

# 创建目录
mkdir -p obj bin system Students/data Students/logs Students/index

# 编译源文件
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/metadata_structures.cpp -o obj/metadata_structures.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/database_registry_manager.cpp -o obj/database_registry_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/system_config_manager.cpp -o obj/system_config_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/database_metadata_manager.cpp -o obj/database_metadata_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c complete_example.cpp -o obj/complete_example.o

# 链接生成可执行文件
g++ obj/*.o -o bin/haodb_complete

echo "编译完成! 可执行文件: bin/haodb_complete"
echo "运行: ./bin/haodb_complete"
*/

//==================== CMakeLists.txt ====================
/*
cmake_minimum_required(VERSION 3.10)
project(HAODB_Metadata)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(include)

# 源文件
set(SOURCES
    src/metadata_structures.cpp
    src/database_registry_manager.cpp
    src/system_config_manager.cpp
    src/database_metadata_manager.cpp
)

# 头文件
set(HEADERS
    include/metadata_structures.h
    include/database_registry_manager.h
    include/system_config_manager.h
    include/database_metadata_manager.h
)

# 创建静态库
add_library(haodb_metadata STATIC ${SOURCES})

# 创建示例可执行文件
add_executable(haodb_complete complete_example.cpp)
target_link_libraries(haodb_complete haodb_metadata)

# 安装规则
install(TARGETS haodb_metadata DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/haodb)
install(TARGETS haodb_complete DESTINATION bin)

# 测试
enable_testing()
add_test(NAME complete_test COMMAND haodb_complete)
*/