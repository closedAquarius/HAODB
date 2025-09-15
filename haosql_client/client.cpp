#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ���ߺ�������������
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

// ���ߺ���������ֱ�� >>END\n
string recvUntilEnd(SOCKET sock) {
    static string buffer; // ���棬��ֹ����
    char temp[4096];

    while (true) {
        size_t pos = buffer.find(">>END\n");
        if (pos != string::npos) {
            string result = buffer.substr(0, pos);
            buffer.erase(0, pos + 6); // �Ƴ��Ѵ�����
            return result;
        }

        memset(temp, 0, sizeof(temp));
        int bytes = recv(sock, temp, sizeof(temp) - 1, 0);
        if (bytes <= 0) throw runtime_error("�������Ͽ�����");
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

    cout << "�����ӵ������� (127.0.0.1:8080)" << endl;

    try {
        // ===== ��¼���� =====
        while (true) {
            // �����˺���ʾ
            string msg = recvUntilEnd(sock);
            cout << msg << endl;
            string account;
            getline(cin, account);
            sendAll(sock, account);

            // ����������ʾ
            msg = recvUntilEnd(sock);
            cout << msg << endl;
            string password;
            getline(cin, password);
            sendAll(sock, password);

            // ���յ�¼���
            msg = recvUntilEnd(sock);
            cout << msg << endl;
            if (msg.find("��ӭ") != string::npos) {
                break; // ��¼�ɹ�
            }
            cout << "�����������¼��Ϣ" << endl;
        }

        // ===== SQL ѭ�� =====
        while (true) {
            // ���շ�������ʾ
            string prompt = recvUntilEnd(sock);
            cout << prompt << endl;

            // �û����� SQL
            string sql;
            getline(cin, sql);
            sendAll(sock, sql);

            if (sql == "exit;") break; // �ͷ���˱���һ��

            // ����ִ�н��
            string result = recvUntilEnd(sock);
            cout << result << endl;
        }

    }
    catch (const exception& e) {
        cerr << "����: " << e.what() << endl;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

