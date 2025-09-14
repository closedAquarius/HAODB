#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 工具函数：接收直到 >>END\n
string recvUntilEnd(SOCKET sock) {
    string result;
    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            throw runtime_error("服务器断开连接");
        }
        result.append(buffer, bytesReceived);

        // 检查结束符
        if (result.size() >= 6 &&
            result.find(">>END\n") != string::npos) {
            // 去掉结束标记
            result.erase(result.find(">>END\n"));
            break;
        }
    }
    return result;
}

int main() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << endl;
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
        while (true) {
            // === 1. 接收服务器消息 ===
            string serverMsg = recvUntilEnd(sock);
            cout << serverMsg;

            // === 2. 用户输入 ===
            string input;
            getline(cin, input);

            send(sock, input.c_str(), static_cast<int>(input.size()), 0);

            if (input == "exit" || input == "quit") {
                cout << "退出客户端" << endl;
                break;
            }
        }
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
