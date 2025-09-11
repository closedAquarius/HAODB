#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include "database_registry_manager.h"
#include "system_config_manager.h"
#include "database_metadata_manager.h"

// ģ�����ݿ�����ĸ�������
void SimulateDatabaseOperations(DatabaseMetadataManager& db_mgr) {
    std::cout << "\n--- ģ�����ݿ���� ---" << std::endl;

    // ģ��һЩI/O����
    db_mgr.UpdateIOStats(1000, 500);  // 1000�ζ�ȡ��500��д��
    db_mgr.UpdateCacheStats(800, 700); // 800�����У�700��δ����

    // ģ��������
    for (int i = 0; i < 100; i++) {
        db_mgr.IncrementTransactionCount();
    }

    // ģ�����ӱ仯
    db_mgr.SetActiveConnections(15);

    // ģ��洢�仯
    db_mgr.UpdateTotalPages(2048, 512);  // ��ҳ��2048������512
    db_mgr.UpdateDatabaseSize(8388608);  // 8MB��С

    std::cout << "ģ��������" << std::endl;
}

// ����Student����ж���
std::vector<ColumnMeta> CreateStudentColumns() {
    std::vector<ColumnMeta> columns;

    // ѧ����
    ColumnMeta sno_col;
    strcpy(sno_col.column_name, "Sno");
    sno_col.column_id = 1;
    sno_col.data_type = 2;  // VARCHAR
    sno_col.max_length = 12;
    sno_col.is_nullable = 0;
    sno_col.is_primary_key = 1;
    sno_col.is_unique = 1;
    sno_col.column_offset = 0;
    columns.push_back(sno_col);

    // ������
    ColumnMeta sname_col;
    strcpy(sname_col.column_name, "Sname");
    sname_col.column_id = 2;
    sname_col.data_type = 2;  // VARCHAR
    sname_col.max_length = 20;
    sname_col.is_nullable = 0;
    sname_col.column_offset = 12;
    columns.push_back(sname_col);

    // �Ա���
    ColumnMeta ssex_col;
    strcpy(ssex_col.column_name, "Ssex");
    ssex_col.column_id = 3;
    ssex_col.data_type = 2;  // VARCHAR
    ssex_col.max_length = 2;
    ssex_col.is_nullable = 1;
    ssex_col.column_offset = 32;
    columns.push_back(ssex_col);

    // ������
    ColumnMeta sage_col;
    strcpy(sage_col.column_name, "Sage");
    sage_col.column_id = 4;
    sage_col.data_type = 1;  // INT
    sage_col.max_length = 4;
    sage_col.is_nullable = 1;
    sage_col.column_offset = 34;
    columns.push_back(sage_col);

    // ����ϵ��
    ColumnMeta sdept_col;
    strcpy(sdept_col.column_name, "Sdept");
    sdept_col.column_id = 5;
    sdept_col.data_type = 2;  // VARCHAR
    sdept_col.max_length = 20;
    sdept_col.is_nullable = 1;
    sdept_col.column_offset = 38;
    columns.push_back(sdept_col);

    return columns;
}

// ����Course����ж���
std::vector<ColumnMeta> CreateCourseColumns() {
    std::vector<ColumnMeta> columns;

    // �γ̺���
    ColumnMeta cno_col;
    strcpy(cno_col.column_name, "Cno");
    cno_col.column_id = 1;
    cno_col.data_type = 2;  // VARCHAR
    cno_col.max_length = 10;
    cno_col.is_nullable = 0;
    cno_col.is_primary_key = 1;
    cno_col.is_unique = 1;
    cno_col.column_offset = 0;
    columns.push_back(cno_col);

    // �γ�����
    ColumnMeta cname_col;
    strcpy(cname_col.column_name, "Cname");
    cname_col.column_id = 2;
    cname_col.data_type = 2;  // VARCHAR
    cname_col.max_length = 50;
    cname_col.is_nullable = 0;
    cname_col.column_offset = 10;
    columns.push_back(cname_col);

    // ���п���
    ColumnMeta cpno_col;
    strcpy(cpno_col.column_name, "Cpno");
    cpno_col.column_id = 3;
    cpno_col.data_type = 2;  // VARCHAR
    cpno_col.max_length = 10;
    cpno_col.is_nullable = 1;
    cpno_col.column_offset = 60;
    columns.push_back(cpno_col);

    // ѧ����
    ColumnMeta ccredit_col;
    strcpy(ccredit_col.column_name, "Ccredit");
    ccredit_col.column_id = 4;
    ccredit_col.data_type = 1;  // INT
    ccredit_col.max_length = 4;
    ccredit_col.is_nullable = 0;
    strcpy(ccredit_col.default_value, "0");
    ccredit_col.column_offset = 70;
    columns.push_back(ccredit_col);

    return columns;
}

// ����SC����ж���
std::vector<ColumnMeta> CreateSCColumns() {
    std::vector<ColumnMeta> columns;

    // ѧ����
    ColumnMeta sno_col;
    strcpy(sno_col.column_name, "Sno");
    sno_col.column_id = 1;
    sno_col.data_type = 2;  // VARCHAR
    sno_col.max_length = 12;
    sno_col.is_nullable = 0;
    sno_col.is_primary_key = 1;  // ��������
    sno_col.column_offset = 0;
    columns.push_back(sno_col);

    // �γ̺���
    ColumnMeta cno_col;
    strcpy(cno_col.column_name, "Cno");
    cno_col.column_id = 2;
    cno_col.data_type = 2;  // VARCHAR
    cno_col.max_length = 10;
    cno_col.is_nullable = 0;
    cno_col.is_primary_key = 1;  // ��������
    cno_col.column_offset = 12;
    columns.push_back(cno_col);

    // �ɼ���
    ColumnMeta grade_col;
    strcpy(grade_col.column_name, "Grade");
    grade_col.column_id = 3;
    grade_col.data_type = 1;  // INT
    grade_col.max_length = 4;
    grade_col.is_nullable = 1;
    grade_col.column_offset = 22;
    columns.push_back(grade_col);

    return columns;
}

/*
int main() {
    std::cout << "=== HAODB ����Ԫ���ݹ���ϵͳ��ʾ ===" << std::endl;
    std::cout << "�汾: 1.0 (�������ù���)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 1. ϵͳ���ù���
    std::cout << "\n1. ϵͳ���ù���" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    SystemConfigManager config_mgr("HAODB\\system\\system_config.hao");
    config_mgr.LoadConfig();

    // ����ϵͳ����
    config_mgr.SetMaxDatabases(20);
    config_mgr.SetMaxConnections(200);
    config_mgr.SetGlobalBufferSize(67108864);  // 64MB
    config_mgr.EnableQueryCache(true);
    config_mgr.EnableAuthentication(true);
    config_mgr.SetSystemPaths("HAODB\\system\\logs\\",
        "HAODB\\temp\\",
        "HAODB\\backup\\");

    config_mgr.ShowConfig();
    config_mgr.SaveConfig();

    // 2. ���ݿ�ע������
    std::cout << "\n2. ���ݿ�ע������" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    DatabaseRegistryManager registry_mgr("HAODB/system/databases.registry");
    registry_mgr.LoadRegistry();

    // ע��Students���ݿ�
    registry_mgr.RegisterDatabase("Students", "HAODB\\Students\\", 1, "admin");
    registry_mgr.UpdateDatabaseStatus("Students", 1);  // ����Ϊ����
    registry_mgr.ListDatabases();

    // 3. �����͹���Students���ݿ�
    std::cout << "\n3. Students���ݿ����" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    DatabaseMetadataManager students_db("HAODB/Students/meta_data.hao");

    // ���Լ��أ����ʧ���򴴽������ݿ�
    if (!students_db.LoadMetadata()) {
        std::cout << "�����µ�Students���ݿ�..." << std::endl;
        students_db.CreateNewDatabase("Students", 1);

        // ������ṹ
        std::cout << "������ṹ..." << std::endl;
        auto student_columns = CreateStudentColumns();
        students_db.CreateTable("Student", 1, student_columns, 8192);

        auto course_columns = CreateCourseColumns();
        students_db.CreateTable("Course", 2, course_columns, 1048576);

        auto sc_columns = CreateSCColumns();
        students_db.CreateTable("SC", 3, sc_columns, 1572864);

        // ��������
        std::cout << "��������..." << std::endl;
        students_db.CreateIndex("pk_student_sno", 1, 1, { 1 }, true, 100);
        students_db.CreateIndex("idx_student_sname", 2, 1, { 2 }, false, 200);
        students_db.CreateIndex("pk_course_cno", 3, 2, { 1 }, true, 300);
        students_db.CreateIndex("pk_sc_sno_cno", 4, 3, { 1, 2 }, true, 400);
        students_db.CreateIndex("idx_sc_sno", 5, 3, { 1 }, false, 500);

        students_db.SaveMetadata();
    }

    // 4. �洢���ù�����ʾ
    std::cout << "\n4. �洢���ù���" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ��ȡ��ǰ�洢����
    const StorageConfig& current_storage = students_db.GetStorageConfig();
    StorageConfigManager storage_mgr(const_cast<StorageConfig&>(current_storage));

    // �����洢����
    students_db.SetBufferPoolSize(67108864);      // 64MB�����
    students_db.SetMaxConnections(100);           // ���100����
    students_db.SetLockTimeout(60);               // 60������ʱ
    students_db.EnableAutoCheckpoint(true);       // �����Զ�����
    students_db.SetPageCacheSize(33554432);       // 32MBҳ�滺��
    students_db.EnableCompression(false);         // ����ѹ��

    // �����ļ�·��
    students_db.SetDataFilePath("HAODB\\Students\\data\\students.dat");
    students_db.SetLogFilePath("HAODB\\Students\\logs\\students.log");
    students_db.SetIndexFilePath("HAODB\\Students\\index\\students.idx");

    students_db.ShowStorageConfig();

    // 5. ��־���ù�����ʾ
    std::cout << "\n5. ��־���ù���" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ������־����
    students_db.SetLogLevel(2);                   // INFO����
    students_db.SetLogFileSize(100);              // 100MB��־�ļ�
    students_db.SetLogFileCount(10);              // ����10���ļ�
    students_db.EnableWAL(true);                  // ����WAL
    students_db.SetWALBufferSize(16777216);       // 16MB WAL������
    students_db.SetSyncMode(1);                   // ͬ��ģʽ
    students_db.EnableLogRotation(true);          // ������־��ת

    students_db.ShowLogConfig();

    // 6. ���ݲ�����ͳ����Ϣ��ʾ
    std::cout << "\n6. ���ݲ�����ͳ����Ϣ" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ���±�����
    students_db.UpdateTableRowCount("Student", 1500);
    students_db.UpdateTableRowCount("Course", 300);
    students_db.UpdateTableRowCount("SC", 8000);

    // ģ�����ݿ����
    SimulateDatabaseOperations(students_db);

    // ���ñ���ʱ��
    students_db.SetLastBackupTime(static_cast<uint64_t>(time(nullptr)));

    students_db.ShowDatabaseStats();

    // 7. ʹ�ø�����������ʾ
    std::cout << "\n7. ������������ʾ" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ��־���ù�����
    const LogConfig& current_log = students_db.GetLogConfig();
    LogConfigManager log_mgr(const_cast<LogConfig&>(current_log));

    std::cout << "��ǰ��־����: " << log_mgr.GetLogLevelString() << std::endl;
    std::cout << "��ǰͬ��ģʽ: " << log_mgr.GetSyncModeString() << std::endl;

    // ͳ����Ϣ������
    const DatabaseStats& current_stats = students_db.GetDatabaseStats();
    DatabaseStatsManager stats_mgr(const_cast<DatabaseStats&>(current_stats));

    std::cout << "����������: " << std::fixed << std::setprecision(2)
        << stats_mgr.GetCacheHitRatio() << "%" << std::endl;
    std::cout << "�洢������: " << std::fixed << std::setprecision(2)
        << stats_mgr.GetStorageUtilization() << "%" << std::endl;

    // 8. ���ܷ�������
    std::cout << "\n8. ���ܷ�������" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.GeneratePerformanceReport();

    // 9. ��ʾ�������ݿ���Ϣ
    std::cout << "\n9. �������ݿ���Ϣ" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.ShowCompleteInfo();

    // 10. �������и���
    std::cout << "\n10. �������ú�Ԫ����" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    students_db.SaveMetadata();

    // 11. ������֤��ʾ
    std::cout << "\n11. ������֤" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ��֤�洢����
    if (storage_mgr.ValidateConfig()) {
        std::cout << "�洢������֤ͨ��" << std::endl;
    }
    else {
        std::cout << "�洢������֤ʧ��" << std::endl;
    }

    // ��֤��־����
    /*if (log_mgr.ValidateConfig()) {
        std::cout << "��־������֤ͨ��" << std::endl;
    }
    else {
        std::cout << "��־������֤ʧ��" << std::endl;
    }*//*

    // 12. ��̬���õ�����ʾ
    std::cout << "\n12. ��̬���õ�����ʾ" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    // ģ������ʱ���õ���
    std::cout << "ģ��߸��س�������������..." << std::endl;
    students_db.SetBufferPoolSize(134217728);    // ���ӵ�128MB
    students_db.SetMaxConnections(200);          // ���ӵ�200����
    students_db.SetLogLevel(1);                  // ���͵�WARN����

    // ģ��һ��ʱ����ͳ��
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    students_db.UpdateIOStats(5000, 2000);       // ����I/O����
    students_db.UpdateCacheStats(4500, 500);     // ���õĻ������
    students_db.SetActiveConnections(45);        // �����Ծ����

    std::cout << "�߸��غ�����ܱ���:" << std::endl;
    students_db.GeneratePerformanceReport();

    // 13. ���ձ��������
    std::cout << "\n13. ���ձ���" << std::endl;
    std::cout << std::string(30, '-') << std::endl;

    if (students_db.SaveMetadata()) {
        std::cout << "�������ú�Ԫ���ݱ���ɹ�" << std::endl;
    }
    else {
        std::cout << "����ʧ��" << std::endl;
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "����Ԫ���ݹ���ϵͳ��ʾ��ɣ�" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // ��ʾ����ͳ����Ϣ
    std::cout << "\n����ͳ����Ϣ:" << std::endl;
    std::cout << "- ����ı�����: " << students_db.GetTableCount() << std::endl;
    std::cout << "- �������������: " << students_db.GetIndexCount() << std::endl;
    std::cout << "- �ܼ�¼��: " << students_db.GetTotalRowCount() << std::endl;
    std::cout << "- ����������: " << std::fixed << std::setprecision(2)
        << students_db.GetCacheHitRatio() << "%" << std::endl;
    std::cout << "- ƽ�������С: " << students_db.GetAverageTransactionSize() << " I/O" << std::endl;

    return 0;
}
*/

//==================== ����ű� compile_complete.sh ====================
/*
#!/bin/bash
# ������Ŀ����ű�

echo "����HAODB����Ԫ���ݹ���ϵͳ..."

# ����Ŀ¼
mkdir -p obj bin system Students/data Students/logs Students/index

# ����Դ�ļ�
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/metadata_structures.cpp -o obj/metadata_structures.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/database_registry_manager.cpp -o obj/database_registry_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/system_config_manager.cpp -o obj/system_config_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c src/database_metadata_manager.cpp -o obj/database_metadata_manager.o
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude -c complete_example.cpp -o obj/complete_example.o

# �������ɿ�ִ���ļ�
g++ obj/*.o -o bin/haodb_complete

echo "�������! ��ִ���ļ�: bin/haodb_complete"
echo "����: ./bin/haodb_complete"
*/

//==================== CMakeLists.txt ====================
/*
cmake_minimum_required(VERSION 3.10)
project(HAODB_Metadata)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ����Ŀ¼
include_directories(include)

# Դ�ļ�
set(SOURCES
    src/metadata_structures.cpp
    src/database_registry_manager.cpp
    src/system_config_manager.cpp
    src/database_metadata_manager.cpp
)

# ͷ�ļ�
set(HEADERS
    include/metadata_structures.h
    include/database_registry_manager.h
    include/system_config_manager.h
    include/database_metadata_manager.h
)

# ������̬��
add_library(haodb_metadata STATIC ${SOURCES})

# ����ʾ����ִ���ļ�
add_executable(haodb_complete complete_example.cpp)
target_link_libraries(haodb_complete haodb_metadata)

# ��װ����
install(TARGETS haodb_metadata DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/haodb)
install(TARGETS haodb_complete DESTINATION bin)

# ����
enable_testing()
add_test(NAME complete_test COMMAND haodb_complete)
*/