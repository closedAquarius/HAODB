# HAOSQL 数据库系统

## 简介

HAOSQL 是一个基于 C++ 实现的简易数据库系统，旨在提供基本 SQL 数据库功能，包括数据定义、数据操作、查询处理和存储管理。该项目包含词法分析、语法分析、语义分析、执行器以及存储管理等模块，实现了基本的数据增删改查功能，并提供简单的事务日志、恢复机制、AI智能纠错。

## 分支介绍
-   **master:** 项目服务器
-   **client:** 项目客户端（默认账号root，1234）

## 目录结构
. ├── HAOSQL.sln # Visual Studio 解决方案文件 
    ├── README.md # 项目说明文档 
    ├── HAOSQL/ # 源代码目录 
        │ ├── AI.cpp # AI 相关代码 (可能用于 SQL 纠错) 
        │ ├── AI.h # AI 头文件 
        │ ├── B+tree.cpp # B+ 树实现 
        │ ├── B+tree.h # B+ 树头文件 
        │ ├── buffer_pool.cpp # 缓冲池管理 
        │ ├── buffer_pool.h # 缓冲池头文件 
        │ ├── catalog_manager.cpp # 目录管理器实现 
        │ ├── catalog_manager.h # 目录管理器头文件 
        │ ├── database_metadata_manager.cpp # 数据库元数据管理实现 
        │ ├── database_metadata_manager.h # 数据库元数据管理头文件 
        │ ├── database_registry_Manager.cpp # 数据库注册管理器实现 
        │ ├── database_registry_manager.h # 数据库注册管理器头文件 
        │ ├── dataType.h # 数据类型定义 
        │ ├── dbname_setter.cpp # 数据库名称设置 
        │ ├── executor.cpp # 执行器实现 
        │ ├── executor.h # 执行器头文件 
        │ ├── file_manager.cpp # 文件管理器实现 
        │ ├── file_manager.h # 文件管理器头文件 
        │ ├── HAOSQL.cpp # 主程序入口 
        │ ├── HAOSQL.vcxproj # Visual Studio 项目文件 
        │ ├── lexer.cpp # 词法分析器实现 
        │ ├── lexer.h # 词法分析器头文件 
        │ ├── login.cpp # 登录管理实现 
        │ ├── login.h # 登录管理头文件 
        │ ├── operator.cpp # 算子实现 
        │ ├── operator.h # 算子头文件 
        │ ├── page.cpp # 页面管理实现 
        │ ├── page.h # 页面管理头文件 
        │ ├── parser.cpp # 语法分析器实现 
        │ ├── parser.h # 语法分析器头文件 
        │ ├── semantic.cpp # 语义分析器实现 
        │ ├── semantic.h # 语义分析器头文件 
        │ ├── sql_grammar.txt # SQL 语法定义 
        │ ├── test_catalog2.cpp # 目录管理测试 
        │ ├── test_log.cpp # 日志系统测试 
        │ └── ... 
    ├── x64/ # 编译输出目录 
        │ └── Debug/ # Debug 版本输出 
        └── ...


## 功能特性

-   **SQL 解析器:**  支持基本 SQL 语法，包括 `CREATE TABLE`, `SELECT`, `INSERT`, `UPDATE`, `DELETE` 等语句。
    -   词法分析器 ([`lexer.cpp`](HAOSQL/lexer.cpp), [`lexer.h`](HAOSQL/lexer.h)): 将 SQL 语句分解为词法单元。
    -   语法分析器 ([`parser.cpp`](HAOSQL/parser.cpp), [`parser.h`](HAOSQL/parser.h)):  根据 SQL 语法规则，将词法单元组织成语法树。
    -   语义分析器 ([`semantic.cpp`](HAOSQL/semantic.cpp), [`semantic.h`](HAOSQL/semantic.h)): 检查 SQL 语句的语义正确性，并生成四元式中间代码。
-   **查询执行器:**  执行 SQL 查询，支持基本的关系代数运算。
    -   执行器 ([`executor.cpp`](HAOSQL/executor.cpp), [`executor.h`](HAOSQL/executor.h)): 负责执行四元式代码，实现数据的检索、更新和删除。
    -   算子 ([`operator.cpp`](HAOSQL/operator.cpp), [`operator.h`](HAOSQL/operator.h)): 实现了关系代数中的各种算子，如选择、投影、连接等。
-   **存储管理:**  提供基本的数据存储和检索功能。
    -   文件管理器 ([`file_manager.cpp`](HAOSQL/file_manager.cpp), [`file_manager.h`](HAOSQL/file_manager.h)):  管理数据库文件的读写操作。
    -   缓冲池管理器 ([`buffer_pool.cpp`](HAOSQL/buffer_pool.cpp), [`buffer_pool.h`](HAOSQL/buffer_pool.h)):  缓存数据库页面，提高数据访问速度。
    -   页面 ([`page.cpp`](HAOSQL/page.cpp), [`page.h`](HAOSQL/page.h)):  数据库存储的基本单元。
-   **目录管理:**  管理数据库的元数据信息。
    -   目录管理器 ([`catalog_manager.cpp`](HAOSQL/catalog_manager.cpp), [`catalog_manager.h`](HAOSQL/catalog_manager.h)): 负责管理数据库、表、索引等元数据信息。
-   **B+ 树索引:**  支持 B+ 树索引，提高查询效率 ([`B+tree.cpp`](HAOSQL/B+tree.cpp), [`B+tree.h`](HAOSQL/B+tree.h))。
-   **日志管理:** 提供了简单的日志记录和恢复机制 ([`test_log.cpp`](HAOSQL/test_log.cpp))。
-   **锁结构设计:** 实现并运用了简答的互斥锁功能([`LockManager.cpp`](HAOSQL/LockManager.cpp), [`LockManager.h`](HAOSQL/LockManager.h))。
-   **多线程处理:** 主函数入口HAOSQL实现了多线程处理，配合互斥锁实习安全访问
-   **集成AI:** 集成AI实现智能纠错功能([`AI.cpp`](HAOSQL/AI.cpp), [`AI.h`](HAOSQL/AI.h))。
-   **用户登录:** 实现了简单的用户登录管理功能 ([`login.cpp`](HAOSQL/login.cpp), [`login.h`](HAOSQL/login.h))。


## 模块详解

1.  **SQL 解析模块**
    -   词法分析：使用 [`Lexer`](HAOSQL/lexer.cpp) 类将输入的 SQL 语句转换为 Token 序列。
    -   语法分析：使用 [`SQLParser`](HAOSQL/parser.cpp) 类将 Token 序列解析为语法树。
    -   语义分析：使用 [`SemanticAnalyzer`](HAOSQL/semantic.cpp) 类进行语义检查，并生成中间代码（四元式）。
    -   关键函数：[`SemanticAnalyzer::analyze`](HAOSQL/semantic.cpp) 是语义分析的入口。
2.  **查询执行模块**
    -   执行器：[`executor.cpp`](HAOSQL/executor.cpp) 中的代码负责执行生成的四元式，实现 SQL 语句的功能。
    -   算子：定义了各种关系代数算子，用于实现查询、插入、更新和删除等操作。
3.  **存储管理模块**
    -   文件管理：[`FileManager`](HAOSQL/file_manager.cpp) 负责数据库文件的读写，包括打开、读取页面、写入页面等操作。
    -   缓冲池管理：[`BufferPoolManager`](HAOSQL/buffer_pool.cpp) 用于缓存数据库页面，减少磁盘 I/O。
    -   页面管理：[`Page`](HAOSQL/page.cpp) 类定义了页面的结构和操作，包括插入记录、读取记录、删除记录等。
4.  **目录管理模块**
    -   [`CatalogManager`](HAOSQL/catalog_manager.cpp) 负责管理数据库的元数据，包括数据库信息、表信息、索引信息等。
5.  **索引管理模块**
    -   [`IndexManager`](HAOSQL/index_manager.h) 负责索引的创建、删除和查询等操作，使用 B+ 树实现索引。
6.  **日志管理模块**
    -   [`test_log.cpp`](HAOSQL/test_log.cpp) 提供了日志记录和恢复机制的测试代码，实现了简单的 WAL（Write-Ahead Logging）功能。
7.  **用户登录模块**
    -   [`LoginManager`](HAOSQL/login.cpp) 实现了用户的注册、登录和登出功能，用于管理数据库用户的身份验证。

## 如何编译和运行

1.  **环境要求:**
    -   Visual Studio (推荐 2019 或更高版本)
    -   C++ 编译器
    -   Windows SDK
2.  **编译步骤:**
    -   使用 Visual Studio 打开 `HAOSQL.sln` 解决方案文件。
    -   选择 "Debug" 或 "Release" 配置，以及 "x64" 平台。
    -   点击 "生成" -> "生成解决方案"。
3.  **运行步骤:**
    -   编译成功后，在 `x64/Debug` 或 `x64/Release` 目录下找到生成的可执行文件 `HAOSQL.exe`。
    -   运行 `HAOSQL.exe`。
    -   根据提示输入 SQL 语句进行操作。
3.  **AI:**
    -   执行`vcpkg install curl nlohmann-json`
    -   [可选]执行`vcpkg install curl:x64-windows nlohmann-json:x64-windows`
    -   集成智谱AI(https://open.bigmodel.cn/) 将`headers = curl_slist_append(headers, "Authorization: Bearer ***");`替换成自己的API KEY



## 使用示例

1.  **创建数据库:**

    ```sql
    CREATE DATABASE TestDB;
    ```

2.  **使用数据库:**

    ```sql
    USE DATABASE TestDB;
    ```

3.  **创建表:**

    ```sql
    CREATE TABLE students (
        id INT PRIMARY KEY,
        name VARCHAR(20) NOT NULL,
        age INT,
        grade VARCHAR(2)
    );
    ```

4.  **插入数据:**

    ```sql
    INSERT INTO students VALUES (1, "Alice", 18, "A");
    ```

5.  **查询数据:**

    ```sql
    SELECT * FROM students WHERE age > 17;
    ```

6.  **更新数据:**

    ```sql
    UPDATE students SET grade = "B" WHERE id = 1;
    ```

7.  **删除数据:**

    ```sql
    DELETE FROM students WHERE id = 1;
    ```


8.  **增加用户:**

    ```sql
    CREATE USER '用户名' IDENTIFIED BY '密码';
    ```

8.  **回滚操作:**

    ```sql
    rollback 2;
    ```

## 测试

项目包含单元测试和集成测试，主要集中在 [`test_catalog2.cpp`](HAOSQL/test_catalog2.cpp) 和 [`test_log.cpp`](HAOSQL/test_log.cpp) 中。

-   [`test_catalog2.cpp`](HAOSQL/test_catalog2.cpp):  测试目录管理器的各项功能，包括数据库管理、表管理、索引管理、统计信息管理等。
    -   主要测试方法：[`CatalogManagerTest::RunAllTests`](HAOSQL/test_catalog2.cpp)
-   [`test_log.cpp`](HAOSQL/test_log.cpp):  测试日志系统的各项功能，包括日志记录、日志查看、日志分析、性能监控等。

## 未来改进方向

-   **更完善的 SQL 支持:**  支持更复杂的 SQL 语法，如子查询、视图等。
-   **事务支持:**  实现完整的 ACID 事务特性。
-   **存储优化:**  优化存储结构，提高数据访问效率。
-   **更完善的测试:** 增加更多的单元测试和集成测试，提高代码质量。

## 贡献

欢迎参与 HAOSQL 项目的开发和改进！

## 许可证

MIT License
