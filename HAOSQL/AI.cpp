//#include <string>
//#include <curl/curl.h>
//#include <nlohmann/json.hpp>
//#include <windows.h>
//#include "AI.h"
//
//using json = nlohmann::json;
//
//// ----------------- CURL 回调函数 -----------------
//static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
//    ((std::string*)userp)->append((char*)contents, size * nmemb);
//    return size * nmemb;
//}
//
//// ----------------- ANSI/GBK -> UTF-8 转换 -----------------
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
//    std::string utf8(lenU8 - 1, '\0'); // 去掉结尾 '\0'
//    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], lenU8, NULL, NULL);
//    return utf8;
//}
//
//// ----------------- 调用智谱 AI -----------------
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
//    // 构造 JSON 请求体
//    json j;
//    j["model"] = "glm-4";
//    j["messages"] = {
//        { {"role", "system"}, {"content", u8"你是一个SQL语法纠错助手。"} },
//        { {"role", "user"}, {"content", u8"帮我纠正这条SQL语句，如果有错直接给我sql语句，没有错返回数字1: " + sql} }
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
//// ----------------- 提取纠正后的 SQL -----------------
//std::string extractCorrectedSQL(const std::string& aiResponse) {
//    try {
//        auto j = json::parse(aiResponse);
//        if (j.contains("choices") && !j["choices"].empty() &&
//            j["choices"][0].contains("message") &&
//            j["choices"][0]["message"].contains("content")) {
//
//            std::string content = j["choices"][0]["message"]["content"];
//
//            // 如果开头是数字 '1'，表示 SQL 无错误
//            if (!content.empty() && content[0] == '1') {
//                return ""; // 没有错误
//            }
//
//            // 查找 ```sql ... ``` 中的 SQL
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
//        std::cerr << "解析AI返回内容出错: " << e.what() << std::endl;
//    }
//    return "";
//}
//
//std::string CALLAI(const std::string& sql) {
//    // 保存当前控制台编码
//    UINT oldOutCP = GetConsoleOutputCP();
//    UINT oldInCP = GetConsoleCP();
//
//    // 切换到 UTF-8
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//    std::string aiResponse = callZhipuAI(sql);
//
//    std::cout << "AI原始返回：" << std::endl << aiResponse << std::endl;
//
//    std::string correctedSQL = extractCorrectedSQL(aiResponse);
//    if (!correctedSQL.empty()) {
//        std::cout << "提取到纠正后的SQL语句：" << std::endl;
//        std::cout << correctedSQL << std::endl;
//    }
//    else {
//        std::cout << "SQL没有错误或无法提取" << std::endl;
//    }
//
//    // 切回原来的编码
//    SetConsoleOutputCP(oldOutCP);
//    SetConsoleCP(oldInCP);
//
//    return correctedSQL;
//}
//
//// ----------------- 主函数示例 -----------------
///*int main() {
//    // 控制台输出 UTF-8
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//    std::string sql = "SELEC name FORM students;"; // 错误 SQL
//    std::string aiResponse = callZhipuAI(sql);
//
//    std::cout << "AI原始返回：" << std::endl << aiResponse << std::endl;
//
//    std::string correctedSQL = extractCorrectedSQL(aiResponse);
//    if (!correctedSQL.empty()) {
//        std::cout << "提取到纠正后的SQL语句：" << std::endl;
//        std::cout << correctedSQL << std::endl;
//    }
//    else {
//        std::cout << "SQL没有错误或无法提取" << std::endl;
//    }
//
//    return 0;
//}*/
