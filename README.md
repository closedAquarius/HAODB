# HaoSQL 客户端

这是一个简单的 C++ 客户端，用于与 HaoSQL 数据库服务器交互。 它允许用户连接到服务器、进行身份验证并执行 SQL 查询。

## 功能

*   连接到 HaoSQL 服务器。
*   处理用户身份验证（用户名和密码）。
*   将 SQL 查询发送到服务器。
*   接收并显示来自服务器的结果。

## 预quisites

*   C++ 编译器（例如，Visual Studio）
*   Windows SDK
*   HaoSQL 服务器正在运行且可访问。

## 构建客户端

1.  在 Visual Studio 中打开 `haosql_client.sln` 解决方案文件。
2.  选择所需的构建配置（Debug 或 Release）和平台（x86 或 x64）。
3.  构建解决方案。

## 配置

默认情况下，客户端连接到 `127.0.0.1:8080` 上的服务器。 可以在 [client.cpp](haosql_client/client.cpp) 的 [`main`](haosql_client/client.cpp) 函数中更改此设置。

## 用法

1.  运行编译后的可执行文件 (`haosql_client.exe`)。
2.  客户端将提示您输入用户名和密码。
3.  成功通过身份验证后，您可以输入 SQL 查询。
4.  要退出，请输入 `exit;` 并按 Enter 键。

## 代码结构

*   [`client.cpp`](haosql_client/client.cpp): 包含主应用程序逻辑，包括套接字连接、身份验证和 SQL 查询执行。
*   [`haosql_client.vcxproj`](haosql_client/haosql_client.vcxproj): Visual Studio 项目文件。
*   [`haosql_client.vcxproj.filters`](haosql_client/haosql_client.vcxproj.filters): Visual Studio 项目筛选器文件。

## 依赖项

*   Winsock2 (`ws2_32.lib`)

## 函数

*   [`sendAll`](haosql_client/client.cpp): 将整个消息发送到套接字。
*   [`recvUntilEnd`](haosql_client/client.cpp): 从套接字接收数据，直到找到 ">>END\\n" 分隔符。

## 错误处理

客户端包含用于套接字操作和服务器通信的基本错误处理。 错误将打印到控制台。

## 注意事项

*   客户端使用简单的协议，其中消息以 ">>END\\n" 结尾。
*   代码是用中文编写的，但可以使用翻译工具翻译注释。

## .gitignore

[.gitignore](.gitignore) 文件指定了 Git 应该忽略的有意未跟踪的文件。 这包括：

*   Visual Studio 构建文件和文件夹
*   可执行文件
*   目标文件
*   补丁文件
*   日志文件