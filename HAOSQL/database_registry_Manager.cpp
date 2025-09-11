#define _CRT_SECURE_NO_WARNINGS
#include "database_registry_manager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

DatabaseRegistryManager::DatabaseRegistryManager(const std::string& file_path)
    : registry_file_path(file_path) {
}

bool DatabaseRegistryManager::LoadRegistry() 
{
    std::ifstream file(registry_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cout << "ע����ļ������ڣ������µ�ע���" << std::endl;
        return CreateNewRegistry();
    }

    // ��ȡͷ����Ϣ
    file.read(reinterpret_cast<char*>(&registry), sizeof(DatabaseRegistry));

    // ��֤�ļ���ʽ
    if (strncmp(registry.magic, "HAODBSYS", 8) != 0) 
    {
        std::cerr << "��Ч��ע����ļ���ʽ" << std::endl;
        return false;
    }

    // ��ȡ���ݿ���Ŀ
    databases.clear();
    databases.resize(registry.database_count);

    for (uint32_t i = 0; i < registry.database_count; i++)
    {
        file.read(reinterpret_cast<char*>(&databases[i]), sizeof(DatabaseEntry));
    }

    file.close();
    std::cout << "�ɹ����� " << registry.database_count << " �����ݿ�ע����Ϣ" << std::endl;
    return true;
}

bool DatabaseRegistryManager::SaveRegistry()
{
    std::ofstream file(registry_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "�޷�д��ע����ļ�: " << registry_file_path << std::endl;
        return false;
    }

    // ����ͷ����Ϣ
    registry.database_count = databases.size();
    registry.last_update = static_cast<uint64_t>(time(nullptr));

    // д��ͷ��
    file.write(reinterpret_cast<const char*>(&registry), sizeof(DatabaseRegistry));

    // д�����ݿ���Ŀ
    for (const auto& db : databases) 
    {
        file.write(reinterpret_cast<const char*>(&db), sizeof(DatabaseEntry));
    }

    file.close();
    std::cout << "ע�����ɹ�" << std::endl;
    return true;
}

bool DatabaseRegistryManager::RegisterDatabase(const std::string& db_name,
    const std::string& db_path,
    uint32_t db_id,
    const std::string& owner) 
{
    // ����Ƿ��Ѵ���
    for (const auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0) 
        {
            std::cerr << "���ݿ� " << db_name << " �Ѵ���" << std::endl;
            return false;
        }
    }

    // ��������Ŀ
    DatabaseEntry new_entry;
    strcpy(new_entry.db_name, db_name.c_str());
    strcpy(new_entry.db_path, db_path.c_str());
    new_entry.db_id = db_id;
    new_entry.status = 1;  // ����
    new_entry.create_time = static_cast<uint64_t>(time(nullptr));
    new_entry.last_access = new_entry.create_time;
    strcpy(new_entry.owner, owner.c_str());
    new_entry.auto_start = 1;

    databases.push_back(new_entry);

    std::cout << "���ݿ� " << db_name << " ע��ɹ�" << std::endl;
    return SaveRegistry();
}

bool DatabaseRegistryManager::UnregisterDatabase(const std::string& db_name)
{
    auto it = std::find_if(databases.begin(), databases.end(),
        [&](const DatabaseEntry& db) 
        {
            return strcmp(db.db_name, db_name.c_str()) == 0;
        });

    if (it != databases.end()) 
    {
        databases.erase(it);
        std::cout << "���ݿ� " << db_name << " ע���ɹ�" << std::endl;
        return SaveRegistry();
    }

    std::cerr << "δ�ҵ����ݿ�: " << db_name << std::endl;
    return false;
}

bool DatabaseRegistryManager::UpdateDatabaseStatus(const std::string& db_name, uint8_t status) {
    for (auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0)
        {
            db.status = status;
            db.last_access = static_cast<uint64_t>(time(nullptr));
            return SaveRegistry();
        }
    }
    return false;
}

DatabaseEntry* DatabaseRegistryManager::GetDatabaseInfo(const std::string& db_name) {
    for (auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0)
        {
            return &db;
        }
    }
    return nullptr;
}

void DatabaseRegistryManager::ListDatabases() 
{
    std::cout << "\n=== ���ݿ�ע��� ===" << std::endl;
    std::cout << "�����ݿ�����: " << databases.size() << std::endl;

    for (const auto& db : databases) 
    {
        std::cout << "ID: " << db.db_id
            << ", ����: " << db.db_name
            << ", ״̬: " << (db.status == 1 ? "����" : "����")
            << ", ������: " << db.owner << std::endl;
    }
}

bool DatabaseRegistryManager::CreateNewRegistry() 
{
    registry = DatabaseRegistry();  // ʹ��Ĭ�Ϲ��캯��
    databases.clear();
    return SaveRegistry();
}