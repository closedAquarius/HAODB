//#include <string>
//#include <curl/curl.h>
//#include <nlohmann/json.hpp>
//#include <windows.h>
//#include "AI.h"
//
//using json = nlohmann::json;
//
//// ----------------- CURL �ص����� -----------------
//static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
//    ((std::string*)userp)->append((char*)contents, size * nmemb);
//    return size * nmemb;
//}
//
//// ----------------- ANSI/GBK -> UTF-8 ת�� -----------------
//std::string ansiToUtf8(const std::string& ansi) {
//    int lenW = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, NULL, 0);
//    if (lenW == 0) return "";
//
//    std::wstring wstr(lenW, L'\0');
//    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, &wstr[0], lenW);
//
//    int lenU8 = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
//    if (lenU8 == 0) return "";
//
//    std::string utf8(lenU8 - 1, '\0'); // ȥ����β '\0'
//    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], lenU8, NULL, NULL);
//    return utf8;
//}
//
//// ----------------- �������� AI -----------------
//std::string callZhipuAI(const std::string& tosql) {
//    CURL* curl = curl_easy_init();
//    std::string readBuffer;
//    if (!curl) return "";
//
//    std::string sql = ansiToUtf8(tosql);
//
//    std::string url = "https://open.bigmodel.cn/api/paas/v4/chat/completions";
//    struct curl_slist* headers = NULL;
//    headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");
//    headers = curl_slist_append(headers, "Authorization: Bearer de9bcb4c9d8c46d5a1854e7ec619feea.kWlX9XpLnD1yfL6F");
//
//    // ���� JSON ������
//    json j;
//    j["model"] = "glm-4";
//    j["messages"] = {
//        { {"role", "system"}, {"content", u8"����һ��SQL�﷨�������֡�"} },
//        { {"role", "user"}, {"content", u8"���Ҿ�������SQL��䣬����д�ֱ�Ӹ���sql��䣬û�д�������1: " + sql} }
//    };
//    std::string jsonData = j.dump();
//
//    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
//
//    CURLcode res = curl_easy_perform(curl);
//    if (res != CURLE_OK) {
//        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
//    }
//    curl_easy_cleanup(curl);
//    if (headers) curl_slist_free_all(headers);
//
//    return readBuffer;
//}
//
//
//// ----------------- ��ȡ������� SQL -----------------
//std::string extractCorrectedSQL(const std::string& aiResponse) {
//    try {
//        auto j = json::parse(aiResponse);
//        if (j.contains("choices") && !j["choices"].empty() &&
//            j["choices"][0].contains("message") &&
//            j["choices"][0]["message"].contains("content")) {
//
//            std::string content = j["choices"][0]["message"]["content"];
//
//            // �����ͷ������ '1'����ʾ SQL �޴���
//            if (!content.empty() && content[0] == '1') {
//                return ""; // û�д���
//            }
//
//            // ���� ```sql ... ``` �е� SQL
//            size_t start = content.find("```sql");
//            size_t end = content.find("```", start + 6);
//            if (start != std::string::npos && end != std::string::npos) {
//                start += 6;
//                if (content[start] == '\n') start++;
//                std::string correctedSQL = content.substr(start, end - start);
//                return correctedSQL;
//            }
//        }
//    }
//    catch (const std::exception& e) {
//        std::cerr << "����AI�������ݳ���: " << e.what() << std::endl;
//    }
//    return "";
//}
//
//std::string CALLAI(const std::string& sql) {
//    // ���浱ǰ����̨����
//    UINT oldOutCP = GetConsoleOutputCP();
//    UINT oldInCP = GetConsoleCP();
//
//    // �л��� UTF-8
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//    std::string aiResponse = callZhipuAI(sql);
//
//    std::cout << "AIԭʼ���أ�" << std::endl << aiResponse << std::endl;
//
//    std::string correctedSQL = extractCorrectedSQL(aiResponse);
//    if (!correctedSQL.empty()) {
//        std::cout << "��ȡ���������SQL��䣺" << std::endl;
//        std::cout << correctedSQL << std::endl;
//    }
//    else {
//        std::cout << "SQLû�д�����޷���ȡ" << std::endl;
//    }
//
//    // �л�ԭ���ı���
//    SetConsoleOutputCP(oldOutCP);
//    SetConsoleCP(oldInCP);
//
//    return correctedSQL;
//}
//
//// ----------------- ������ʾ�� -----------------
///*int main() {
//    // ����̨��� UTF-8
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//    std::string sql = "SELEC name FORM students;"; // ���� SQL
//    std::string aiResponse = callZhipuAI(sql);
//
//    std::cout << "AIԭʼ���أ�" << std::endl << aiResponse << std::endl;
//
//    std::string correctedSQL = extractCorrectedSQL(aiResponse);
//    if (!correctedSQL.empty()) {
//        std::cout << "��ȡ���������SQL��䣺" << std::endl;
//        std::cout << correctedSQL << std::endl;
//    }
//    else {
//        std::cout << "SQLû�д�����޷���ȡ" << std::endl;
//    }
//
//    return 0;
//}*/
