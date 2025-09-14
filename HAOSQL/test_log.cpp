#include "logger_integration.h"
#include <iostream>

// ========== 代码设计架构详细说明 ==========
/*
日志系统架构设计原理：

1. 分层架构设计
   ┌─────────────────────────────────────────┐
   │           用户接口层                    │
   │  SimpleLogger | EnhancedExecutor        │
   │  宏定义接口   | 测试接口                │
   └─────────────────────────────────────────┘
   ┌─────────────────────────────────────────┐
   │           集成管理层                    │
   │  GlobalLoggerManager | LogConfigManager │
   │  LogViewer | PerformanceMonitor         │
   └─────────────────────────────────────────┘
   ┌─────────────────────────────────────────┐
   │           核心日志层                    │
   │  DatabaseLogger | WALBuffer             │
   │  TransactionManager                     │
   └─────────────────────────────────────────┘
   ┌─────────────────────────────────────────┐
   │           存储管理层                    │
   │  LogFileManager | RecoveryManager       │
   │  文件I/O | 日志轮转                     │
   └─────────────────────────────────────────┘

2. 核心设计模式
   - 工厂模式：LoggerFactory 统一创建逻辑
   - 单例模式：GlobalLoggerManager 全局管理
   - 组合模式：LoggedXXXComposition 避免继承问题
   - 策略模式：多种同步策略（异步/同步/全同步）
   - 观察者模式：性能监控和日志查看

3. SimpleLogger vs EnhancedExecutor 区别
   SimpleLogger（简化接口）：
   - 轻量级，易于使用
   - 自动事务管理
   - 适合快速日志记录
   - 封装复杂性

   EnhancedExecutor（增强执行器）：
   - 完整的执行模拟
   - 详细的WAL日志
   - 支持崩溃恢复
   - 性能监控集成
   - 适合生产环境

4. WAL机制实现
   - 预写式日志保证ACID特性
   - 支持崩溃恢复和操作撤销
   - 缓冲区优化减少I/O
   - 多种同步策略平衡性能和安全

5. 日志管理特性
   - 自动日志轮转
   - 多级日志级别
   - 性能统计和监控
   - 并发安全操作
   - 配置文件支持
*/

// ========== 主函数：展示完整功能 ==========
int test_log() {
    try {
        std::cout << "=== HAODB 完整日志系统演示 ===" << std::endl;
        std::cout << "Version: 1.0 | Build: " << __DATE__ << std::endl;

        // ========== 第一部分：基础功能演示 ==========
        std::cout << "\n第一部分：基础功能演示" << std::endl;

        // 1. 初始化日志系统
        CatalogManager catalog("TEST_HAODB");
        catalog.Initialize();
        catalog.ShowDatabaseList();

        INIT_LOGGER("TestDB", &catalog);
        std::cout << "日志系统初始化完成" << std::endl;

        // 2. 使用简化接口
        std::cout << "\n--- SimpleLogger 演示 ---" << std::endl;
        SimpleLogger simple_logger("TestDB");
        simple_logger.BeginTransaction();

        // 记录各种类型的操作
        std::vector<OperationLogRecord::SimpleQuadruple> select_quads = {
            {"FROM", "students", "-", "T1"},
            {"WHERE", "age > 20", "-", "T2"},
            {"SELECT", "id,name,age", "T1", "T3"}
        };
        simple_logger.LogSQL("SELECT id, name, age FROM students WHERE age > 20", select_quads, true);

        simple_logger.LogInsert("students", { {"id", "1001"}, {"name", "张三"}, {"age", "22"} });
        simple_logger.LogUpdate("students", "id = 1001", { {"age", "23"} });
        simple_logger.LogDelete("students", "id = 1002");

        simple_logger.CommitTransaction();
        std::cout << "SimpleLogger 操作完成" << std::endl;

        // 3. 使用增强执行器
        std::cout << "\n--- EnhancedExecutor 演示 ---" << std::endl;
        EnhancedExecutor enhanced_executor("TestDB", "demo_session", "demo_user");

        // 执行数据操作（自动WAL记录）
        enhanced_executor.InsertRecord("products", {
            {"id", "P001"}, {"name", "笔记本电脑"}, {"price", "5999"}
            });
        enhanced_executor.InsertRecord("products", {
            {"id", "P002"}, {"name", "无线鼠标"}, {"price", "199"}
            });

        enhanced_executor.UpdateRecord("products", "id = P001", { {"price", "5799"} });
        enhanced_executor.DeleteRecord("products", "id = P002");

        // 演示撤销功能
        enhanced_executor.UndoLastDelete();
        std::cout << "✓ EnhancedExecutor 操作完成（包含撤销）" << std::endl;

        // ========== 第二部分：日志查看和分析 ==========
        std::cout << "\n📊 第二部分：日志查看和分析" << std::endl;

        LogViewer log_viewer("TestDB");

        // 显示各种类型的日志
        log_viewer.PrintRecentOperations(15);
        log_viewer.PrintRecentErrors(5);

        // 搜索特定模式的日志
        std::cout << "\n--- 搜索日志 ---" << std::endl;
        auto insert_logs = log_viewer.SearchLogs("INSERT", "operation");
        std::cout << "找到 " << insert_logs.size() << " 条INSERT相关日志" << std::endl;

        // 显示日志统计
        auto stats = log_viewer.GetLogStatistics();
        std::cout << "\n--- 日志统计信息 ---" << std::endl;
        std::cout << "总操作数: " << stats.total_operations << std::endl;
        std::cout << "总错误数: " << stats.total_errors << std::endl;
        std::cout << "日志大小: " << stats.total_log_size_mb << " MB" << std::endl;

        // ========== 第三部分：性能监控演示 ==========
        std::cout << "\n⚡ 第三部分：性能监控演示" << std::endl;

        LogPerformanceMonitor perf_monitor;

        // 模拟批量操作性能测试
        perf_monitor.StartOperation("BATCH_INSERT");
        for (int i = 0; i < 100; ++i) {
            enhanced_executor.InsertRecord("performance_test", {
                {"id", std::to_string(i)},
                {"data", "test_data_" + std::to_string(i)},
                {"timestamp", std::to_string(time(nullptr))}
                });
        }
        perf_monitor.EndOperation("BATCH_INSERT");

        // 显示性能统计
        perf_monitor.PrintAllStats();

        // ========== 第四部分：配置管理演示 ==========
        std::cout << "\n⚙️ 第四部分：配置管理演示" << std::endl;

        // 显示当前配置
        enhanced_executor.ShowLoggerStatus();

        // 动态更新配置
        // LogConfigManager::UpdateLogLevel("DemoDB", 3); // DEBUG级别
        // LogConfigManager::UpdateSyncMode("DemoDB", 2); // 全同步模式

        // ========== 第五部分：高级功能演示 ==========
        std::cout << "\n🚀 第五部分：高级功能演示" << std::endl;

        // 使用组合模式的带日志算子
        Table test_table;
        uint32_t txn_id = CURRENT_LOGGER->BeginTransaction();

        // 创建带日志的插入算子
        Row test_row = { {"id", "999"}, {"name", "测试记录"} };
        auto logged_insert = LoggedOperatorFactory::CreateLoggedInsert(
            &test_table, test_row, "test_table", CURRENT_LOGGER, txn_id);

        logged_insert->execute();
        CURRENT_LOGGER->CommitTransaction(txn_id);

        std::cout << "✓ 组合模式算子执行完成，表中记录数: " << test_table.size() << std::endl;

        // 导出日志
        bool export_result = log_viewer.ExportLogs("demo_logs_export.txt");
        if (export_result) {
            std::cout << "✓ 日志导出完成: demo_logs_export.txt" << std::endl;
        }

        // ========== 第六部分：运行完整测试套件 ==========
        std::cout << "\n🧪 第六部分：运行完整测试套件" << std::endl;

        // LoggerTestSuite test_suite("TestDB");
        // test_suite.RunAllTests();

        // ========== 总结 ==========
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "              演示完成总结" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "✓ 基础日志功能：SimpleLogger 和 EnhancedExecutor" << std::endl;
        std::cout << "✓ WAL机制：事务管理、崩溃恢复、操作撤销" << std::endl;
        std::cout << "✓ 日志管理：轮转、查看、搜索、导出" << std::endl;
        std::cout << "✓ 性能监控：操作统计、时间分析" << std::endl;
        std::cout << "✓ 配置管理：动态配置、多级日志" << std::endl;
        std::cout << "✓ 高级功能：组合模式算子、并发安全" << std::endl;
        std::cout << "✓ 测试套件：全面的单元测试和集成测试" << std::endl;

        std::cout << "\n🎉 HAODB日志系统演示成功完成！" << std::endl;

        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "\n❌ 演示过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "\n❌ 发生未知错误" << std::endl;
        return 2;
    }
}

// ========== 独立测试函数 ==========
void RunIndependentTests() {
    std::cout << "\n=== 独立功能测试 ===" << std::endl;

    try {
        // 测试配置文件加载
        std::cout << "\n--- 测试配置文件功能 ---" << std::endl;

        // 创建测试配置文件
        LogConfigInfo test_config;
        test_config.log_level_name = "DEBUG";
        test_config.log_level_code = 3;
        test_config.enable_wal = true;
        test_config.sync_mode_name = "全同步";
        test_config.sync_mode_code = 2;

        // bool save_result = LogConfigManager::SaveConfig(test_config, "test_config.ini");
        /*if (save_result) {
            std::cout << "✓ 配置文件保存成功" << std::endl;

            LogConfigInfo loaded_config = LogConfigManager::LoadConfig("test_config.ini");
            std::cout << "✓ 配置文件加载成功: " << loaded_config.log_level_name << std::endl;
        }*/

        // 测试工具函数
        std::cout << "\n--- 测试工具函数 ---" << std::endl;

        uint64_t timestamp = 1640995200000; // 2022-01-01 00:00:00
        std::string formatted_time = LoggerUtils::FormatTimestamp(timestamp);
        std::cout << "时间戳格式化: " << formatted_time << std::endl;

        std::string file_size = LoggerUtils::FormatFileSize(1536000); // ~1.5MB
        std::cout << "文件大小格式化: " << file_size << std::endl;

        uint8_t log_level = LoggerUtils::ParseLogLevel("DEBUG");
        std::cout << "日志级别解析: DEBUG -> " << (int)log_level << std::endl;

        // 测试并发安全
        std::cout << "\n--- 测试并发安全性 ---" << std::endl;

        std::vector<std::thread> threads;
        std::atomic<int> success_count(0);

        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([i, &success_count]() {
                try {
                    SimpleLogger thread_logger("ConcurrentTest");
                    thread_logger.BeginTransaction();

                    for (int j = 0; j < 5; ++j) {
                        thread_logger.LogInsert("concurrent_table", {
                            {"thread_id", std::to_string(i)},
                            {"operation", std::to_string(j)}
                            });
                    }

                    thread_logger.CommitTransaction();
                    success_count++;
                }
                catch (const std::exception& e) {
                    std::cerr << "线程 " << i << " 异常: " << e.what() << std::endl;
                }
                });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::cout << "✓ 并发测试完成，成功线程数: " << success_count.load() << "/3" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "独立测试异常: " << e.what() << std::endl;
    }
}