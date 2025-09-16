#pragma once
#include <iomanip>
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <utility>

using Row = std::map<std::string, std::string>;

class TableFormatter {
private:
    // ANSI颜色码
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string CYAN = "\033[36m";
    const std::string BLUE = "\033[34m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string WHITE = "\033[37m";
    const std::string BG_BLUE = "\033[44m";

public:
    // 计算每列的最大宽度
    std::map<std::string, int> calculateColumnWidths(const std::vector<std::string>& columns,
        const std::vector<Row>& result) {
        std::map<std::string, int> widths;

        // 初始化为列名长度
        for (const auto& col : columns) {
            widths[col] = static_cast<int>(col.length());
        }

        // 检查数据行的最大长度
        for (const auto& row : result) {
            for (const auto& col : columns) {
                auto it = row.find(col);
                if (it != row.end()) {
                    int dataLen = static_cast<int>(it->second.length());
                    widths[col] = (std::max)(widths[col], dataLen);
                }
            }
        }

        // 设置最小宽度和最大宽度
        for (auto& pair : widths) {
            pair.second = (std::max)(pair.second, 6);  // 最小宽度6
            pair.second = (std::min)(pair.second, 25); // 最大宽度25
        }

        return widths;
    }

    // 格式化完整表格
    std::string formatTable(const std::vector<std::string>& columns,
        const std::vector<Row>& result) {
        if (result.empty()) {
            return "";
        }

        auto widths = calculateColumnWidths(columns, result);
        std::stringstream ss;

        // 标题
        ss << "\n" << CYAN << BOLD << "查询结果: " << RESET << "\n\n";

        // 顶部边框
        ss << CYAN << "┌";
        for (size_t i = 0; i < columns.size(); ++i) {
            ss << std::string(widths[columns[i]] + 2, '-');
            if (i < columns.size() - 1) 
                ss << "┬";
        }
        ss << "┐" << RESET << "\n";

        // 表头
        ss << CYAN << "│" << RESET;
        for (const auto& col : columns) {
            ss << BLUE << BOLD << " " << std::setw(widths[col]) << std::left << col << " " << RESET;
            ss << CYAN << "│" << RESET;

            cout << "column: " << col << endl;
        }
        ss << "\n";

        // 中间分隔线
        ss << CYAN << "├";
        for (size_t i = 0; i < columns.size(); ++i) {
            ss << std::string(widths[columns[i]] + 2, '-');
            if (i < columns.size() - 1) ss << "┼";
        }
        ss << "┤" << RESET << "\n";

        // 数据行
        for (size_t rowIdx = 0; rowIdx < result.size(); ++rowIdx) {
            const auto& row = result[rowIdx];
            std::string bgColor = (rowIdx % 2 == 0) ? "" : "\033[48;5;237m"; // 隔行变色

            ss << CYAN << "│" << RESET;
            for (const auto& col : columns) {
                ss << bgColor << " ";
                auto it = row.find(col);
                std::string value = (it != row.end()) ? it->second : "NULL";

                // 截断过长字符串
                if (value.length() > static_cast<size_t>(widths[col])) {
                    value = value.substr(0, widths[col] - 3) + "...";
                }

                cout << "row: " << value << endl;

                ss << std::setw(widths[col]) << std::left << value << " " << RESET;
                ss << CYAN << "│" << RESET;
            }
            ss << "\n";
        }

        // 底部边框
        ss << CYAN << "└";
        for (size_t i = 0; i < columns.size(); ++i) {
            ss << std::string(widths[columns[i]] + 2, '-');
            if (i < columns.size() - 1) ss << "┴";
        }
        ss << "┘" << RESET << "\n";

        // 统计信息
        ss << "\n" << GREEN << BOLD << "查询成功完成！" << RESET << "\n";
        ss << YELLOW << "返回 " << BOLD << result.size() << " 行数据" << RESET << "\n";
        ss << GREEN << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
        ss << "\n" << YELLOW << "HAODB> " << RESET;

        return ss.str();
    }
};

class LogFormatter {
private:
    // ANSI 颜色码
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string CYAN = "\033[36m";
    const std::string BLUE = "\033[34m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string RED = "\033[31m";
    const std::string MAGENTA = "\033[35m";
    const std::string WHITE = "\033[37m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_CYAN = "\033[46m";

public:
    // 获取操作类型的彩色标识和图标
    std::pair<std::string, std::string> getOperationStyle(const std::string& operation) {
        if (operation.find("INSERT") != std::string::npos) {
            return { GREEN + BOLD + " ADD " + RESET, "新增记录" };
        }
        else if (operation.find("DELETE") != std::string::npos) {
            return { RED + BOLD + " DEL " + RESET, "删除记录" };
        }
        else if (operation.find("UPDATE") != std::string::npos) {
            return { YELLOW + BOLD + " UPD " + RESET, "更新记录" };
        }
        else if (operation.find("SELECT") != std::string::npos) {
            return { BLUE + BOLD + " SEL " + RESET, "查询记录" };
        }
        else if (operation.find("CREATE") != std::string::npos) {
            return { CYAN + BOLD + " CRT " + RESET, "创建对象" };
        }
        else if (operation.find("DROP") != std::string::npos) {
            return { MAGENTA + BOLD + " DRP " + RESET, "删除对象" };
        }
        else {
            return { WHITE + BOLD + " OTH " + RESET, "其他操作" };
        }
    }

    // 生成美观的日志表格
    std::string formatLogTable(const std::vector<std::string>& operations, int steps) {
        std::ostringstream oss;

        // 标题区域
        oss << "\n" << CYAN << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
        oss << YELLOW << BOLD << "                                   操作历史记录                                \n" << RESET;
        oss << CYAN << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;

        oss << GREEN << "\n显示最近 " << BOLD << steps << RESET << GREEN << " 步操作记录\n" << RESET;

        if (operations.empty()) {
            oss << "\n" << YELLOW << "┌" << std::string(77, '-') << "┐\n" << RESET;
            oss << YELLOW << "│" << RESET << WHITE << BOLD << "                                  暂无操作记录                               " << RESET << YELLOW << "│\n" << RESET;
            oss << YELLOW << "│" << RESET << "                                                                             " << YELLOW << "│\n" << RESET;
            oss << YELLOW << "│" << RESET << CYAN << "                       执行一些数据库操作后即可查看历史记录                  " << RESET << YELLOW << "│\n" << RESET;
            oss << YELLOW << "└" << std::string(77, '-') << "┘\n" << RESET;
            return oss.str();
        }

        // 表头
        oss << CYAN << "┌" << std::string(6, '-') << "┬" << std::string(10, '-') << "┬" << std::string(59, '-') << "┐\n" << RESET;
        oss << CYAN << "│" << RESET << BLUE << BOLD << " 序号 " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "   类型   " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "                          操作内容                         " << RESET << CYAN << "│\n" << RESET;

        oss << CYAN << "├" << std::string(6, '-') << "┼" << std::string(10, '-') << "┼" << std::string(59, '-') << "┤\n" << RESET;

        // 数据行
        int line_num = 1;
        for (const auto& op : operations) {
            auto [typeStr, description] = getOperationStyle(op);

            // 序号列
            oss << CYAN << "│" << RESET << WHITE << "   " << line_num << "  " << RESET << CYAN << "│" << RESET;

            // 类型列
            oss << "  " << typeStr << "   " << CYAN << "│" << RESET;

            // 操作内容列 - 处理长文本
            std::string content = op;
            if (content.length() > 57) {
                content = content.substr(0, 54) + "...";
            }
            oss << " " << std::setw(57) << std::left << content << " " << CYAN << "│\n" << RESET;

            line_num++;
        }

        // 表格底部
        oss << CYAN << "└" << std::string(6, '-') << "┴" << std::string(10, '-') << "┴" << std::string(59, '-') << "┘\n" << RESET;

        return oss.str();
    }

    // 生成撤销确认提示
    std::string formatRollbackPrompt(const std::vector<std::string>& operations, int steps) {
        std::ostringstream oss;

        // 撤销警告
        oss << "\n" << RED << BOLD << "撤销操作警告: " << RESET;
        oss << YELLOW << BOLD << "即将撤销以上 " << steps << " 步操作，此操作不可恢复！  " << "\n" << RESET;

        oss << "\n" << CYAN << BOLD << "请确认是否继续？" << RESET << "\n";
        oss << GREEN << "[Y/y] - 确认撤销" << RESET << "  |  " << RED << "[N/n] - 取消操作" << RESET << "\n";

        return oss.str();
    }
};

class TableListFormatter {
private:
    // ANSI 颜色码
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string CYAN = "\033[36m";
    const std::string BLUE = "\033[34m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string RED = "\033[31m";
    const std::string MAGENTA = "\033[35m";
    const std::string WHITE = "\033[37m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_CYAN = "\033[46m";

public:
    // 将时间戳转换为可读格式 - 跨平台安全版本
    std::string formatTimestamp(uint64_t timestamp) {
        if (timestamp == 0) return "N/A";

        time_t rawtime = static_cast<time_t>(timestamp);
        struct tm timeinfo;
        char buffer[20];

#ifdef _WIN32
        // Windows 使用 localtime_s
        if (localtime_s(&timeinfo, &rawtime) != 0) {
            return "N/A";
        }
#else
        // Linux/Unix 使用 localtime_r
        if (localtime_r(&rawtime, &timeinfo) == nullptr) {
            return "N/A";
        }
#endif

        strftime(buffer, sizeof(buffer), "%m-%d %H:%M", &timeinfo);
        return std::string(buffer);
    }

    // 格式化数字显示（添加千位分隔符）
    std::string formatNumber(uint64_t number) {
        if (number == 0) return "0";

        std::string str = std::to_string(number);
        std::string result;
        int count = 0;

        for (int i = str.length() - 1; i >= 0; i--) {
            if (count > 0 && count % 3 == 0) {
                result = "," + result;
            }
            result = str[i] + result;
            count++;
        }

        return result;
    }

    // 获取表状态和颜色
    std::pair<std::string, std::string> getTableStatus(const TableInfo& table) {
        if (table.row_count == 0) {
            return { YELLOW + "空表" + RESET, "EMPTY" };
        }
        else if (table.row_count < 100) {
            return { GREEN + "小表" + RESET, "SMALL" };
        }
        else if (table.row_count < 10000) {
            return { CYAN + "中表" + RESET, "MEDIUM" };
        }
        else {
            return { BLUE + "大表" + RESET, "LARGE" };
        }
    }

    // 生成表格列表的完整显示
    std::string formatTableList(const std::vector<TableInfo>& tables, const std::string& dbName) {
        std::ostringstream oss;

        // 标题区域
        oss << "\n" << CYAN << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
        oss << YELLOW << BOLD << "                      数据库 " << dbName << " 表列表                      \n" << RESET;
        oss << CYAN << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;

        // 检查是否有表
        if (tables.empty()) {
            oss << "\n" << YELLOW << "┌" << std::string(62, '-') << "┐\n" << RESET;
            oss << YELLOW << "│" << RESET << WHITE << BOLD << "                     数据库中暂无表格                         " << RESET << YELLOW << "│\n" << RESET;
            oss << YELLOW << "│" << RESET << CYAN << "              使用 CREATE TABLE 语句创建新表                  " << RESET << YELLOW << "│\n" << RESET;
            oss << YELLOW << "└" << std::string(62, '-') << "┘\n" << RESET;

            oss << GREEN << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
            return oss.str();
        }

        oss << GREEN << "\n共找到 " << BOLD << tables.size() << RESET << GREEN << " 个表\n\n" << RESET;

        // 表头 - 5列布局：ID、表名、状态、列数、创建时间
        oss << CYAN << "┌" << std::string(4, '-') << "┬" << std::string(20, '-') << "┬" << std::string(10, '-')
            << "┬" << std::string(8, '-') << "┬" << std::string(15, '-') << "┐\n" << RESET;

        oss << CYAN << "│" << RESET << BLUE << BOLD << " ID " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "      表名称        " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "   状态   " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "  列数  " << RESET << CYAN << "│" << RESET;
        oss << BLUE << BOLD << "    创建时间   " << RESET << CYAN << "│\n" << RESET;

        oss << CYAN << "├" << std::string(4, '-') << "┼" << std::string(20, '-') << "┼" << std::string(10, '-')
            << "┼" << std::string(8, '-') << "┼" << std::string(15, '-') << "┤\n" << RESET;

        // 数据行 - 修复循环和格式
        uint64_t totalRows = 0;
        uint32_t totalColumns = 0;
        int emptyTables = 0, smallTables = 0, mediumTables = 0, largeTables = 0;

        for (size_t i = 0; i < tables.size(); ++i) {
            const auto& table = tables[i];
            auto [statusStr, statusType] = getTableStatus(table);

            // 统计
            totalRows += table.row_count;
            totalColumns += table.column_count;
            if (table.row_count == 0) emptyTables++;
            else if (table.row_count < 100) smallTables++;
            else if (table.row_count < 10000) mediumTables++;
            else largeTables++;

            // 隔行变色背景
            std::string bgColor = (i % 2 == 0) ? "" : "\033[48;5;237m";

            oss << CYAN << "│" << RESET << bgColor;

            // ID列
            oss << std::setw(3) << table.table_id << " " << RESET << CYAN << "│" << RESET << bgColor;

            // 表名列（截断过长名称）
            std::string tableName = table.table_name;
            if (tableName.length() > 18) {
                tableName = tableName.substr(0, 15) + "...";
            }
            oss << " " << std::setw(18) << std::left << tableName << " " << RESET << CYAN << "│" << RESET << bgColor;

            // 状态列
            oss << "   " << statusStr << "   " << RESET << CYAN << "│" << RESET << bgColor;

            // 列数列
            oss << std::setw(7) << std::right << table.column_count << " " << RESET << CYAN << "│" << RESET << bgColor;

            // 创建时间列
            std::string createTime = formatTimestamp(table.create_time);
            oss << " " << std::setw(13) << std::left << createTime << " " << RESET << CYAN << "│\n" << RESET;
        }

        // 表格底部
        oss << CYAN << "└" << std::string(4, '-') << "┴" << std::string(20, '-') << "┴" << std::string(10, '-')
            << "┴" << std::string(8, '-') << "┴" << std::string(15, '-') << "┘\n" << RESET;

        oss << GREEN << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;

        return oss.str();
    }
};
