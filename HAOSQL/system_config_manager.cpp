#include "system_config_manager.h"
#include <iostream>
#include <fstream>
#include <cstring>

SystemConfigManager::SystemConfigManager(const std::string& file_path)
    : config_file_path(file_path) {
}

bool SystemConfigManager::LoadConfig() 
{
    std::ifstream file(config_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cout << "�����ļ������ڣ�����Ĭ������" << std::endl;
        return CreateDefaultConfig();
    }

    file.read(reinterpret_cast<char*>(&config), sizeof(SystemConfig));

    // ��֤�ļ���ʽ
    if (strncmp(config.magic, "HAOSYSCF", 8) != 0) 
    {
        std::cerr << "��Ч�������ļ���ʽ" << std::endl;
        return false;
    }

    file.close();
    std::cout << "ϵͳ���ü��سɹ�" << std::endl;
    return true;
}

bool SystemConfigManager::SaveConfig() 
{
    std::ofstream file(config_file_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "�޷�д�������ļ�: " << config_file_path << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&config), sizeof(SystemConfig));
    file.close();

    std::cout << "ϵͳ���ñ���ɹ�" << std::endl;
    return true;
}

void SystemConfigManager::SetMaxDatabases(uint32_t max_db) 
{
    config.max_databases = max_db;
    std::cout << "������ݿ���������Ϊ: " << max_db << std::endl;
}

void SystemConfigManager::SetMaxConnections(uint32_t max_connections) 
{
    config.max_connections_total = max_connections;
    std::cout << "�������������Ϊ: " << max_connections << std::endl;
}

void SystemConfigManager::SetGlobalBufferSize(uint32_t buffer_size)
{
    config.global_buffer_size = buffer_size;
    std::cout << "ȫ�ֻ�������С����Ϊ: " << buffer_size << " bytes" << std::endl;
}

void SystemConfigManager::SetCheckpointInterval(uint32_t interval) 
{
    config.checkpoint_interval = interval;
    std::cout << "����������Ϊ: " << interval << " ��" << std::endl;
}

void SystemConfigManager::SetSessionTimeout(uint32_t timeout)
{
    config.session_timeout = timeout;
    std::cout << "�Ự��ʱ����Ϊ: " << timeout << " ��" << std::endl;
}

void SystemConfigManager::SetSystemPaths(const std::string& log_path,
    const std::string& temp_path,
    const std::string& backup_path)
{
    strcpy(config.system_log_path, log_path.c_str());
    strcpy(config.temp_path, temp_path.c_str());
    strcpy(config.backup_path, backup_path.c_str());
    std::cout << "ϵͳ·�����ø���" << std::endl;
}

void SystemConfigManager::EnableQueryCache(bool enable) 
{
    config.enable_query_cache = enable ? 1 : 0;
    std::cout << "��ѯ����" << (enable ? "����" : "����") << std::endl;
}

void SystemConfigManager::EnableAuthentication(bool enable) 
{
    config.enable_authentication = enable ? 1 : 0;
    std::cout << "�����֤" << (enable ? "����" : "����") << std::endl;
}

void SystemConfigManager::EnableAuditLog(bool enable) 
{
    config.enable_audit_log = enable ? 1 : 0;
    std::cout << "�����־" << (enable ? "����" : "����") << std::endl;
}

void SystemConfigManager::ShowConfig() 
{
    std::cout << "\n=== ϵͳ���� ===" << std::endl;
    std::cout << "�汾: " << config.version << std::endl;
    std::cout << "������ݿ���: " << config.max_databases << std::endl;
    std::cout << "���������: " << config.max_connections_total << std::endl;
    std::cout << "Ĭ��ҳ���С: " << config.default_page_size << " bytes" << std::endl;
    std::cout << "Ĭ���ַ���: " << config.default_charset << std::endl;
    std::cout << "ȫ�ֻ�����: " << config.global_buffer_size << " bytes" << std::endl;
    std::cout << "������: " << config.checkpoint_interval << " ��" << std::endl;
    std::cout << "��ѯ����: " << (config.enable_query_cache ? "����" : "����") << std::endl;
    std::cout << "�����֤: " << (config.enable_authentication ? "����" : "����") << std::endl;
}

bool SystemConfigManager::CreateDefaultConfig() 
{
    config = SystemConfig();  // ʹ��Ĭ�Ϲ��캯��
    // ����Ĭ��·��
    strcpy(config.system_log_path, "C:\\HAODB\\system\\logs\\");
    strcpy(config.temp_path, "C:\\HAODB\\temp\\");
    strcpy(config.backup_path, "C:\\HAODB\\backup\\");
    return SaveConfig();
}