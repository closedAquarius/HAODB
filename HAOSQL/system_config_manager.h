#pragma once
#include "metadata_structures.h"
#include <string>

class SystemConfigManager 
{
private:
    std::string config_file_path;
    SystemConfig config;

    bool CreateDefaultConfig();

public:
    explicit SystemConfigManager(const std::string& file_path);

    // �ļ�����
    bool LoadConfig();
    bool SaveConfig();

    // �����޸�
    void SetMaxDatabases(uint32_t max_db);
    void SetMaxConnections(uint32_t max_connections);
    void SetGlobalBufferSize(uint32_t buffer_size);
    void SetCheckpointInterval(uint32_t interval);
    void SetSessionTimeout(uint32_t timeout);
    void SetSystemPaths(const std::string& log_path,
        const std::string& temp_path,
        const std::string& backup_path);

    // ��������
    void EnableQueryCache(bool enable);
    void EnableAuthentication(bool enable);
    void EnableAuditLog(bool enable);

    // ��ѯ�ӿ�
    const SystemConfig& GetConfig() const { return config; }
    void ShowConfig();

    // �����������ȡ
    uint32_t GetMaxDatabases() const { return config.max_databases; }
    uint32_t GetMaxConnections() const { return config.max_connections_total; }
    uint32_t GetGlobalBufferSize() const { return config.global_buffer_size; }
    bool IsQueryCacheEnabled() const { return config.enable_query_cache != 0; }
    bool IsAuthenticationEnabled() const { return config.enable_authentication != 0; }
};
