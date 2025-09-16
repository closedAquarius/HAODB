#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>  // Windows socket
#include <ws2tcpip.h>
// Linux下使用: #include <sys/socket.h>

#pragma comment(lib, "ws2_32.lib")  // Windows下链接socket库

class HAODBStartup {
public:
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

public:
    // 生成顶部装饰字符串
    std::string getHeader() {
        std::stringstream ss;
        /*ss << CYAN << BOLD;
        ss << "╔══════════════════════════════════════════════════════════════════════╗\n";
        ss << "║                                                                      ║\n";
        ss << "╚══════════════════════════════════════════════════════════════════════╝\n";
        ss << RESET;*/

        ss << CYAN << R"(
   ╔══════════════════════════════════════════════════════════════╗

     ██████╗███████╗██╗   ██╗    ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔════╝██║   ██║    ╚════██╗██╔═████╗╚════██╗██╔════╝
    ██║     ███████╗██║   ██║     █████╔╝██║██╔██║ █████╔╝███████╗
    ██║     ╚════██║██║   ██║    ██╔═══╝ ████╔╝██║██╔═══╝ ╚════██║
    ╚██████╗███████║╚██████╔╝    ███████╗╚██████╔╝███████╗███████║
     ╚═════╝╚══════╝ ╚═════╝     ╚══════╝ ╚═════╝ ╚══════╝╚══════╝

   ╚══════════════════════════════════════════════════════════════╝
)" << RESET;
        return ss.str();
    }

    // 生成主LOGO字符串
    std::string getMainLogo() {
        std::stringstream ss;
        ss << BLUE << BOLD;
        ss << R"(
                ██╗  ██╗ █████╗  ██████╗ ██████╗ ██████╗ 
                ██║  ██║██╔══██╗██╔═══██╗██╔══██╗██╔══██╗
                ███████║███████║██║   ██║██║  ██║██████╔╝
                ██╔══██║██╔══██║██║   ██║██║  ██║██╔══██╗
                ██║  ██║██║  ██║╚██████╔╝██████╔╝██████╔╝
                ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═════╝ 
)" << RESET;

        // 添加立体阴影效果
        ss << CYAN;
        ss << "                           ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓\n";
        ss << "                            ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓\n";
        ss << RESET;
        return ss.str();
    }

    // 生成系统信息字符串
    std::string getSystemInfo() {
        std::stringstream ss;
        ss << YELLOW;
        ss << "                             ┌─────────────────────────────────┐\n";
        ss << "                             │  High-Performance Database      │\n";
        ss << "                             │  Management System (HAODB)      │\n";
        ss << "                             └─────────────────────────────────┘\n";
        ss << RESET;
        return ss.str();
    }

    // 生成欢迎信息字符串（接收用户名参数）
    std::string getWelcomeMessage(const std::string& username = "") {
        std::stringstream ss;
        ss << GREEN;
        ss << "\n";
        ss << "    ◆ System Status: " << BOLD << "READY" << RESET << GREEN << "\n";
        ss << "    ◆ SQL Engine: " << BOLD << "INITIALIZED" << RESET << GREEN << "\n";
        ss << "    ◆ Query Parser: " << BOLD << "LOADED" << RESET << GREEN << "\n";
        ss << "\n";
        ss << CYAN;
        ss << "    ═══════════════════════════════════════════════════════\n";

        if (!username.empty()) {
            ss << "     欢迎您，" << BOLD << username << RESET << CYAN << " ！登录成功！\n";
        }
        ss << "     Enter your SQL commands.\n";
        ss << "     Type 'exit' to quit the system.\n";
        ss << "    ═══════════════════════════════════════════════════════\n";
        ss << RESET;
        return ss.str();
    }

    // 生成使用规范信息
    std::string getUsageRules() {
        std::stringstream ss;
        ss << WHITE << BOLD;
        ss << "-------------------------------------------------------------------------------\n";
        ss << "- 名称规范 : 数据库名、表名、列名只能包含字母、数字和下划线，长度不超过63字符 -\n";
        ss << "- 数据类型 : 支持 \"INT\", \"VARCHAR\", \"DECIMAL\", \"DATETIME\"                     -\n";
        ss << "- 主键约束 : 主键列自动设为不可空                                             -\n";
        ss << "- 帮助 : exit退出数据库系统                                                   -\n";
        ss << "-------------------------------------------------------------------------------\n";
        ss << RESET;
        return ss.str();
    }

    // 生成完整的登录成功界面
    std::string getLoginSuccessScreen(const std::string& username) {
        std::stringstream ss;

        // 添加清屏指令（客户端可以识别并执行）
        ss << "\033[2J\033[H";  // ANSI清屏指令

        ss << getHeader();
        ss << getMainLogo();
        ss << getSystemInfo();
        ss << getWelcomeMessage(username);
        ss << "\n" << YELLOW << "HAODB> " << RESET;

        return ss.str();
    }

    // 生成简洁版欢迎信息（原版本兼容）
    std::string getSimpleWelcome(const std::string& username) {
        std::stringstream ss;
        ss << getUsageRules();
        ss << GREEN << BOLD << "🎉 欢迎您，" << username << " ！" << RESET;
        return ss.str();
    }

    // 生成分隔线
    std::string getSeparator() {
        return CYAN + "────────────────────────────────────────────────────────────\n" + RESET;
    }

    // 生成提示符
    std::string getPrompt() {
        return YELLOW + "HAODB> " + RESET;
    }
};