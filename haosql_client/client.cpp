#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 工具函数：完整发送
bool sendAll(SOCKET sock, const string& msg) {
    const char* data = msg.c_str();
    int totalSent = 0;
    int len = static_cast<int>(msg.size());

    while (totalSent < len) {
        int sent = send(sock, data + totalSent, len - totalSent, 0);
        if (sent == SOCKET_ERROR) return false;
        totalSent += sent;
    }
    return true;
}

// 工具函数：接收直到 >>END\n
string recvUntilEnd(SOCKET sock) {
    static string buffer; // 缓存，防止丢包
    char temp[4096];

    while (true) {
        size_t pos = buffer.find(">>END\n");
        if (pos != string::npos) {
            string result = buffer.substr(0, pos);
            buffer.erase(0, pos + 6); // 移除已处理部分
            return result;
        }

        memset(temp, 0, sizeof(temp));
        int bytes = recv(sock, temp, sizeof(temp) - 1, 0);
        if (bytes <= 0) throw runtime_error("服务器断开连接");
        buffer.append(temp, bytes);
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "socket failed, error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "connect failed, error: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "已连接到服务器 (127.0.0.1:8080)" << endl;

    try {
        // ===== 登录流程 =====
        while (true) {
            // 接收账号提示
            string msg = recvUntilEnd(sock);
            cout << msg << endl;
            string account;
            getline(cin, account);
            sendAll(sock, account);

            // 接收密码提示
            msg = recvUntilEnd(sock);
            cout << msg << endl;
            string password;
            getline(cin, password);
            sendAll(sock, password);

            // 接收登录结果
            msg = recvUntilEnd(sock);
            cout << msg << endl;
            if (msg.find("欢迎") != string::npos) {
                break; // 登录成功
            }
            cout << "请重新输入登录信息" << endl;
        }

        // ===== SQL 循环 =====
        while (true) {
            // 接收服务器提示
            string prompt = recvUntilEnd(sock);
            cout << prompt << endl;

            // 用户输入 SQL
            string sql;
            getline(cin, sql);
            sendAll(sock, sql);

            if (sql == "exit;") break; // 和服务端保持一致

            // 接收执行结果
            string result = recvUntilEnd(sock);
            cout << result << endl;
        }

    }
    catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

