#define _CRT_SECURE_NO_WARNINGS
#include "catalog_manager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

CatalogManager::CatalogManager(const std::string& haodb_path)
    : haodb_root_path(haodb_path) {
}

CatalogManager::~CatalogManager() {
    Shutdown();
}

// ==================== ��ʼ�������� ====================

bool CatalogManager::Initialize() {
    try {
        // ����HAODB��Ŀ¼��ϵͳĿ¼
        if (!fs::exists(haodb_root_path)) {
            fs::create_directories(haodb_root_path);
            std::cout << "����HAODB��Ŀ¼: " << haodb_root_path << std::endl;
        }

        std::string system_dir = haodb_root_path + "\\system";
        if (!fs::exists(system_dir)) {
            fs::create_directories(system_dir);
            std::cout << "����ϵͳĿ¼: " << system_dir << std::endl;
        }

        // ��ʼ��ϵͳ���ù�����
        std::string config_file = system_dir + "\\system_config.hao";
        system_config = std::make_unique<SystemConfigManager>(config_file);
        if (!system_config->LoadConfig()) {
            std::cerr << "ϵͳ���ü���ʧ��" << std::endl;
            return false;
        }

        // ��ʼ�����ݿ�ע��������
        std::string registry_file = system_dir + "\\databases.registry";
        registry_manager = std::make_unique<DatabaseRegistryManager>(registry_file);
        if (!registry_manager->LoadRegistry()) {
            std::cerr << "���ݿ�ע������ʧ��" << std::endl;
            return false;
        }

        std::cout << "CatalogManager��ʼ���ɹ�" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "CatalogManager��ʼ��ʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::Shutdown() {
    try {
        // �����������ݿ��Ԫ����
        for (auto& pair : database_managers) {
            if (pair.second) {
                pair.second->SaveMetadata();
            }
        }
        database_managers.clear();

        // ����ϵͳ���ú�ע���
        if (system_config) {
            system_config->SaveConfig();
        }
        if (registry_manager) {
            registry_manager->SaveRegistry();
        }

        std::cout << "CatalogManager�رճɹ�" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "CatalogManager�ر�ʧ��: " << e.what() << std::endl;
        return false;
    }
}

// ==================== ���ݿ���� ====================

bool CatalogManager::CreateDatabase(const std::string& db_name, const std::string& owner) {
    if (!ValidateDatabaseName(db_name)) {
        std::cerr << "��Ч�����ݿ�����: " << db_name << std::endl;
        return false;
    }

    if (DatabaseExists(db_name)) {
        std::cerr << "���ݿ��Ѵ���: " << db_name << std::endl;
        return false;
    }

    try {
        // ����Ŀ¼�ṹ
        if (!CreateDirectoryStructure(db_name)) {
            return false;
        }

        // �����µ����ݿ�ID
        uint32_t db_id = GenerateNewDatabaseId();
        std::string db_path = haodb_root_path + "\\" + db_name;

        // ��ע�����ע�����ݿ�
        if (!registry_manager->RegisterDatabase(db_name, db_path, db_id, owner)) {
            RemoveDirectoryStructure(db_name);
            return false;
        }

        // �������ݿ�Ԫ�����ļ�
        std::string metadata_file = db_path + "\\meta_data.hao";
        auto db_manager = std::make_unique<DatabaseMetadataManager>(metadata_file);

        if (!db_manager->CreateNewDatabase(db_name, db_id)) {
            registry_manager->UnregisterDatabase(db_name);
            RemoveDirectoryStructure(db_name);
            return false;
        }

        // ����Ĭ���ļ�·��
        std::string db_name_lower = db_name;
        std::transform(db_name_lower.begin(), db_name_lower.end(), db_name_lower.begin(), ::tolower);

        db_manager->SetDataFilePath(db_path + "\\data\\" + db_name_lower + ".dat");
        db_manager->SetLogFilePath(db_path + "\\logs\\" + db_name_lower + ".log");
        db_manager->SetIndexFilePath(db_path + "\\index\\" + db_name_lower + ".idx");

        db_manager->SaveMetadata();

        // �������ݿ������
        database_managers[db_name] = std::move(db_manager);

        std::cout << "���ݿ� " << db_name << " �����ɹ�" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "�������ݿ�ʧ��: " << e.what() << std::endl;
        RemoveDirectoryStructure(db_name);
        return false;
    }
}

std::vector<DatabaseInfo> CatalogManager::ListDatabases(const std::string& owner) {
    std::vector<DatabaseInfo> result;

    const auto& databases = registry_manager->GetDatabases();
    for (const auto& db_entry : databases) {
        // ���ָ�����û����������
        if (!owner.empty() && strcmp(db_entry.owner, owner.c_str()) != 0) {
            continue;
        }

        DatabaseInfo info;
        info.db_name = db_entry.db_name;
        info.db_path = db_entry.db_path;
        info.db_id = db_entry.db_id;
        info.owner = db_entry.owner;
        info.create_time = db_entry.create_time;
        info.last_access = db_entry.last_access;

        // ״̬ת��
        switch (db_entry.status) {
        case 0: info.status = "����"; break;
        case 1: info.status = "����"; break;
        case 2: info.status = "ά����"; break;
        default: info.status = "δ֪"; break;
        }

        // ��ȡ�������ʹ�С��Ϣ
        if (LoadDatabaseManager(info.db_name)) {
            auto& db_manager = database_managers[info.db_name];
            info.table_count = db_manager->GetTableCount();
            const auto& stats = db_manager->GetDatabaseStats();
            info.total_size = stats.total_size;
        }
        else {
            info.table_count = 0;
            info.total_size = 0;
        }

        result.push_back(info);
    }

    return result;
}

bool CatalogManager::DropDatabase(const std::string& db_name) {
    if (!DatabaseExists(db_name)) {
        std::cerr << "���ݿⲻ����: " << db_name << std::endl;
        return false;
    }

    try {
        // �ӻ������Ƴ�
        database_managers.erase(db_name);

        // ��ע�����ע��
        if (!registry_manager->UnregisterDatabase(db_name)) {
            std::cerr << "��ע���ע�����ݿ�ʧ��" << std::endl;
            return false;
        }

        // ɾ��Ŀ¼�ṹ
        if (!RemoveDirectoryStructure(db_name)) {
            std::cerr << "ɾ�����ݿ�Ŀ¼ʧ��" << std::endl;
            return false;
        }

        std::cout << "���ݿ� " << db_name << " ɾ���ɹ�" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "ɾ�����ݿ�ʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::DatabaseExists(const std::string& db_name) {
    return registry_manager->GetDatabaseInfo(db_name) != nullptr;
}

DatabaseInfo CatalogManager::GetDatabaseInfo(const std::string& db_name) {
    auto db_entry = registry_manager->GetDatabaseInfo(db_name);
    DatabaseInfo info;

    if (db_entry) {
        info.db_name = db_entry->db_name;
        info.db_path = db_entry->db_path;
        info.db_id = db_entry->db_id;
        info.owner = db_entry->owner;
        info.create_time = db_entry->create_time;
        info.last_access = db_entry->last_access;

        switch (db_entry->status) {
        case 0: info.status = "����"; break;
        case 1: info.status = "����"; break;
        case 2: info.status = "ά����"; break;
        default: info.status = "δ֪"; break;
        }

        if (LoadDatabaseManager(db_name)) {
            auto& db_manager = database_managers[db_name];
            info.table_count = db_manager->GetTableCount();
            const auto& stats = db_manager->GetDatabaseStats();
            info.total_size = stats.total_size;
        }
    }

    return info;
}

// ==================== ���ݱ���� ====================

bool CatalogManager::CreateTable(const std::string& db_name, const std::string& table_name,
    const std::vector<std::vector<std::string>>& column_specs) {
    if (!DatabaseExists(db_name)) {
        std::cerr << "���ݿⲻ����: " << db_name << std::endl;
        return false;
    }

    if (!ValidateTableName(table_name)) {
        std::cerr << "��Ч�ı���: " << table_name << std::endl;
        return false;
    }

    if (!LoadDatabaseManager(db_name)) {
        std::cerr << "�������ݿ������ʧ��" << std::endl;
        return false;
    }

    auto& db_manager = database_managers[db_name];

    // �����Ƿ��Ѵ���
    if (db_manager->GetTableInfo(table_name)) {
        std::cerr << "���Ѵ���: " << table_name << std::endl;
        return false;
    }

    try {
        // �����ж���
        std::vector<ColumnMeta> columns;
        for (size_t i = 0; i < column_specs.size(); i++) {
            if (column_specs[i].size() < 3) {
                std::cerr << "�ж��岻��������Ҫ[����, ����, ����]" << std::endl;
                return false;
            }

            ColumnMeta column;
            strncpy(column.column_name, column_specs[i][0].c_str(), sizeof(column.column_name) - 1);
            column.column_name[sizeof(column.column_name) - 1] = '\0';

            column.column_id = i + 1;
            column.data_type = ParseDataType(column_specs[i][1]);
            column.max_length = std::stoul(column_specs[i][2]);
            column.is_nullable = 1;      // Ĭ�Ͽɿ�
            column.is_primary_key = 0;   // Ĭ�Ϸ�����
            column.is_unique = 0;        // Ĭ�ϲ�Ψһ
            column.column_offset = 0;    // ��������
            memset(column.default_value, 0, sizeof(column.default_value));

            columns.push_back(column);
        }

        // ����ƫ����
        UpdateColumnOffset(columns);

        // �����µı�ID
        uint32_t table_id = GenerateNewTableId(db_name);

        // ������
        if (!db_manager->CreateTable(table_name, table_id, columns, 0)) {
            return false;
        }

        db_manager->SaveMetadata();
        std::cout << "�� " << table_name << " �����ɹ�" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "������ʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::SetPrimaryKey(const std::string& db_name, const std::string& table_name,
    const std::string& column_name, bool is_primary) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    ColumnMeta* column = db_manager->GetColumnInfo(table_name, column_name);

    if (!column) {
        std::cerr << "δ�ҵ���: " << column_name << std::endl;
        return false;
    }

    column->is_primary_key = is_primary ? 1 : 0;
    if (is_primary) {
        column->is_nullable = 0;  // ��������Ϊ��
    }

    db_manager->SaveMetadata();
    std::cout << "�� " << column_name << " ������������Ϊ: " << (is_primary ? "��" : "��") << std::endl;
    return true;
}

bool CatalogManager::SetNullable(const std::string& db_name, const std::string& table_name,
    const std::string& column_name, bool is_nullable) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    ColumnMeta* column = db_manager->GetColumnInfo(table_name, column_name);

    if (!column) {
        std::cerr << "δ�ҵ���: " << column_name << std::endl;
        return false;
    }

    // ������������Ϊ�ɿ�
    if (column->is_primary_key && is_nullable) {
        std::cerr << "�����в�������Ϊ�ɿ�" << std::endl;
        return false;
    }

    column->is_nullable = is_nullable ? 1 : 0;
    db_manager->SaveMetadata();
    std::cout << "�� " << column_name << " �ɿ���������Ϊ: " << (is_nullable ? "��" : "��") << std::endl;
    return true;
}

bool CatalogManager::SetUnique(const std::string& db_name, const std::string& table_name,
    const std::string& column_name, bool is_unique) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    ColumnMeta* column = db_manager->GetColumnInfo(table_name, column_name);

    if (!column) {
        std::cerr << "δ�ҵ���: " << column_name << std::endl;
        return false;
    }

    column->is_unique = is_unique ? 1 : 0;
    db_manager->SaveMetadata();
    std::cout << "�� " << column_name << " Ψһ��������Ϊ: " << (is_unique ? "��" : "��") << std::endl;
    return true;
}

bool CatalogManager::AddColumns(const std::string& db_name, const std::string& table_name,
    const std::vector<std::vector<std::string>>& column_specs) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    TableMeta* table = db_manager->GetTableInfo(table_name);

    if (!table) {
        std::cerr << "δ�ҵ���: " << table_name << std::endl;
        return false;
    }

    try {
        for (const auto& spec : column_specs) {
            if (spec.size() < 3) {
                std::cerr << "�ж��岻����" << std::endl;
                return false;
            }

            ColumnMeta column;
            strncpy(column.column_name, spec[0].c_str(), sizeof(column.column_name) - 1);
            column.column_name[sizeof(column.column_name) - 1] = '\0';

            column.column_id = table->column_count + 1;
            column.data_type = ParseDataType(spec[1]);
            column.max_length = std::stoul(spec[2]);
            column.is_nullable = 1;
            column.is_primary_key = 0;
            column.is_unique = 0;
            column.column_offset = table->columns[table->column_count - 1].column_offset
                + table->columns[table->column_count - 1].max_length;
            memset(column.default_value, 0, sizeof(column.default_value));

            if (!db_manager->AddColumn(table_name, column)) {
                return false;
            }
        }

        db_manager->SaveMetadata();
        std::cout << "�ɹ���� " << column_specs.size() << " �е��� " << table_name << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "�����ʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::DropColumns(const std::string& db_name, const std::string& table_name,
    const std::vector<std::string>& column_names) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];

    for (const auto& column_name : column_names) {
        if (!db_manager->DropColumn(table_name, column_name)) {
            std::cerr << "ɾ����ʧ��: " << column_name << std::endl;
            return false;
        }
    }

    db_manager->SaveMetadata();
    std::cout << "�ɹ�ɾ�� " << column_names.size() << " ��" << std::endl;
    return true;
}

bool CatalogManager::DropTable(const std::string& db_name, const std::string& table_name) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];

    if (!db_manager->DropTable(table_name)) {
        return false;
    }

    db_manager->SaveMetadata();
    return true;
}

std::vector<TableInfo> CatalogManager::ListTables(const std::string& db_name) {
    std::vector<TableInfo> result;

    if (!LoadDatabaseManager(db_name)) {
        return result;
    }

    auto& db_manager = database_managers[db_name];
    const auto& tables = db_manager->GetTables();

    for (const auto& table : tables) {
        TableInfo info;
        info.table_name = table.table_name;
        info.table_id = table.table_id;
        info.row_count = table.row_count;
        info.column_count = table.column_count;
        info.create_time = table.create_time;
        info.data_file_offset = table.data_file_offset;

        // ת������Ϣ
        for (uint32_t i = 0; i < table.column_count; i++) {
            const auto& col = table.columns[i];
            ColumnDefinition col_def(col.column_name, DataTypeToString(col.data_type), col.max_length);
            col_def.is_primary_key = col.is_primary_key != 0;
            col_def.is_nullable = col.is_nullable != 0;
            col_def.is_unique = col.is_unique != 0;
            col_def.default_value = col.default_value;
            col_def.column_offset = col.column_offset;
            info.columns.push_back(col_def);
        }

        result.push_back(info);
    }

    return result;
}

TableInfo CatalogManager::GetTableInfo(const std::string& db_name, const std::string& table_name) {
    TableInfo info;

    if (!LoadDatabaseManager(db_name)) {
        return info;
    }

    auto& db_manager = database_managers[db_name];
    TableMeta* table = db_manager->GetTableInfo(table_name);

    if (table) {
        info.table_name = table->table_name;
        info.table_id = table->table_id;
        info.row_count = table->row_count;
        info.column_count = table->column_count;
        info.create_time = table->create_time;
        info.data_file_offset = table->data_file_offset;

        for (uint32_t i = 0; i < table->column_count; i++) {
            const auto& col = table->columns[i];
            ColumnDefinition col_def(col.column_name, DataTypeToString(col.data_type), col.max_length);
            col_def.is_primary_key = col.is_primary_key != 0;
            col_def.is_nullable = col.is_nullable != 0;
            col_def.is_unique = col.is_unique != 0;
            col_def.default_value = col.default_value;
            col_def.column_offset = col.column_offset;
            info.columns.push_back(col_def);
        }
    }

    return info;
}

bool CatalogManager::TableExists(const std::string& db_name, const std::string& table_name) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->GetTableInfo(table_name) != nullptr;
}

bool CatalogManager::UpdateTableRowCount(const std::string& db_name, const std::string& table_name, uint64_t row_count) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    bool success = db_manager->UpdateTableRowCount(table_name, row_count);
    if (success) {
        db_manager->SaveMetadata();
    }
    return success;
}

// ==================== �������� ====================

bool CatalogManager::CreateIndex(const std::string& db_name, const std::string& table_name,
    const std::string& index_name, const std::vector<std::string>& column_names,
    bool is_unique) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    TableMeta* table = db_manager->GetTableInfo(table_name);

    if (!table) {
        std::cerr << "δ�ҵ���: " << table_name << std::endl;
        return false;
    }

    // ������ID
    std::vector<uint32_t> column_ids;
    for (const auto& col_name : column_names) {
        bool found = false;
        for (uint32_t i = 0; i < table->column_count; i++) {
            if (strcmp(table->columns[i].column_name, col_name.c_str()) == 0) {
                column_ids.push_back(table->columns[i].column_id);
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "δ�ҵ���: " << col_name << std::endl;
            return false;
        }
    }

    uint32_t index_id = GenerateNewIndexId(db_name);
    uint64_t root_page_id = 1;  // ��ʱֵ��ʵ��Ӧ���ɴ洢�������

    if (!db_manager->CreateIndex(index_name, index_id, table->table_id, column_ids, is_unique, root_page_id)) {
        return false;
    }

    db_manager->SaveMetadata();
    std::cout << "���� " << index_name << " �����ɹ�" << std::endl;
    return true;
}

bool CatalogManager::DropIndex(const std::string& db_name, const std::string& index_name) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];

    if (!db_manager->DropIndex(index_name)) {
        return false;
    }

    db_manager->SaveMetadata();
    return true;
}

std::vector<IndexInfo> CatalogManager::ListIndexes(const std::string& db_name, const std::string& table_name) {
    std::vector<IndexInfo> result;

    if (!LoadDatabaseManager(db_name)) {
        return result;
    }

    auto& db_manager = database_managers[db_name];
    const auto& indexes = db_manager->GetIndexes();

    for (const auto& index : indexes) {
        // ���ָ���˱����������
        if (!table_name.empty()) {
            TableMeta* table = db_manager->GetTableInfo(index.table_id);
            if (!table || strcmp(table->table_name, table_name.c_str()) != 0) {
                continue;
            }
        }

        IndexInfo info;
        info.index_name = index.index_name;
        info.index_id = index.index_id;
        info.is_unique = index.is_unique != 0;
        info.root_page_id = index.root_page_id;

        // ��ȡ����������
        TableMeta* table = db_manager->GetTableInfo(index.table_id);
        if (table) {
            info.table_name = table->table_name;

            for (uint32_t i = 0; i < index.column_count; i++) {
                for (uint32_t j = 0; j < table->column_count; j++) {
                    if (table->columns[j].column_id == index.columns[i]) {
                        info.column_names.push_back(table->columns[j].column_name);
                        break;
                    }
                }
            }
        }

        result.push_back(info);
    }

    return result;
}

bool CatalogManager::IndexExists(const std::string& db_name, const std::string& index_name) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->GetIndexInfo(index_name) != nullptr;
}

std::vector<IndexInfo> CatalogManager::FindIndexesWithColumns(
    const std::string& db_name,
    const std::string& table_name,
    const std::vector<std::string>& column_names)
{
    std::vector<IndexInfo> result;

    // ʹ�� ListIndexes ��ȡ��������
    std::vector<IndexInfo> indexes = ListIndexes(db_name, table_name);

    // ���������������ת��һ�����򼯺����ڱȽ�
    std::unordered_set<std::string> target_columns(column_names.begin(), column_names.end());

    // ������������
    for (const auto& index_info : indexes) {
        // ���������������ϻ������ڱȽ�
        std::unordered_set<std::string> index_columns(index_info.column_names.begin(), index_info.column_names.end());

        // �����������������ȣ����ʾ���������������
        if (index_columns == target_columns) {
            result.push_back(index_info);
        }
    }

    return result;
}

// ==================== ͳ����Ϣ���� ====================

bool CatalogManager::UpdateTotalPages(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    const auto& stats = db_manager->GetDatabaseStats();
    return db_manager->UpdateTotalPages(stats.total_pages + increment, stats.free_pages);
}

bool CatalogManager::UpdateFreePages(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    const auto& stats = db_manager->GetDatabaseStats();
    return db_manager->UpdateTotalPages(stats.total_pages, stats.free_pages + increment);
}

bool CatalogManager::UpdateDatabaseSize(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    const auto& stats = db_manager->GetDatabaseStats();
    return db_manager->UpdateDatabaseSize(stats.total_size + increment);
}

bool CatalogManager::UpdateTransactionCount(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    for (uint64_t i = 0; i < increment; i++) {
        db_manager->IncrementTransactionCount();
    }
    return true;
}

bool CatalogManager::UpdateReadCount(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->UpdateIOStats(increment, 0);
}

bool CatalogManager::UpdateWriteCount(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->UpdateIOStats(0, increment);
}

bool CatalogManager::UpdateCacheHits(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->UpdateCacheStats(increment, 0);
}

bool CatalogManager::UpdateCacheMisses(const std::string& db_name, uint64_t increment) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->UpdateCacheStats(0, increment);
}

bool CatalogManager::UpdateActiveConnections(const std::string& db_name, uint32_t count) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    return db_manager->SetActiveConnections(count);
}

DatabaseStatistics CatalogManager::GetDatabaseStatistics(const std::string& db_name) {
    DatabaseStatistics result = {};

    if (!LoadDatabaseManager(db_name)) {
        return result;
    }

    auto& db_manager = database_managers[db_name];
    const auto& stats = db_manager->GetDatabaseStats();

    result.total_pages = stats.total_pages;
    result.free_pages = stats.free_pages;
    result.total_size = stats.total_size;
    result.transaction_count = stats.transaction_count;
    result.total_reads = stats.total_reads;
    result.total_writes = stats.total_writes;
    result.cache_hits = stats.cache_hits;
    result.cache_misses = stats.cache_misses;
    result.active_connections = stats.active_connections;
    result.cache_hit_ratio = db_manager->GetCacheHitRatio();

    if (stats.total_pages > 0) {
        result.storage_utilization = (double)(stats.total_pages - stats.free_pages) / stats.total_pages * 100.0;
    }

    return result;
}

bool CatalogManager::ResetStatistics(const std::string& db_name) {
    if (!LoadDatabaseManager(db_name)) {
        return false;
    }

    auto& db_manager = database_managers[db_name];
    db_manager->ResetStatistics();
    db_manager->SaveMetadata();
    return true;
}

// ==================== ��ʾ��Ϣ ====================

void CatalogManager::ShowSystemInfo() {
    std::cout << "\n=== HAODB ϵͳ��Ϣ ===" << std::endl;
    system_config->ShowConfig();

    //std::cout << "\n=== ���ݿ��б� ===" << std::endl;
    registry_manager->ListDatabases();
}

void CatalogManager::ShowDatabaseList() {
    auto databases = ListDatabases();

    std::cout << "\n=== ���ݿ��б� ===" << std::endl;
    std::cout << std::left << std::setw(15) << "���ݿ���"
        << std::setw(10) << "״̬"
        << std::setw(15) << "������"
        << std::setw(8) << "������"
        << std::setw(12) << "��С(bytes)" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    for (const auto& db : databases) {
        std::cout << std::left << std::setw(15) << db.db_name
            << std::setw(10) << db.status
            << std::setw(15) << db.owner
            << std::setw(8) << db.table_count
            << std::setw(12) << db.total_size << std::endl;
    }
}

void CatalogManager::ShowTableList(const std::string& db_name) {
    auto tables = ListTables(db_name);

    std::cout << "\n=== ���ݿ� " << db_name << " ���б� ===" << std::endl;
    std::cout << std::left << std::setw(20) << "����"
        << std::setw(8) << "��ID"
        << std::setw(8) << "����"
        << std::setw(12) << "����" << std::endl;
    std::cout << std::string(48, '-') << std::endl;

    for (const auto& table : tables) {
        std::cout << std::left << std::setw(20) << table.table_name
            << std::setw(8) << table.table_id
            << std::setw(8) << table.column_count
            << std::setw(12) << table.row_count << std::endl;
    }
}

void CatalogManager::ShowTableStructure(const std::string& db_name, const std::string& table_name) {
    auto table = GetTableInfo(db_name, table_name);

    if (table.table_name.empty()) {
        std::cerr << "δ�ҵ���: " << table_name << std::endl;
        return;
    }

    std::cout << "\n=== ��ṹ: " << table_name << " ===" << std::endl;
    std::cout << "��ID: " << table.table_id << ", ����: " << table.row_count << std::endl;
    std::cout << std::left << std::setw(15) << "����"
        << std::setw(10) << "����"
        << std::setw(8) << "����"
        << std::setw(6) << "����"
        << std::setw(6) << "�ɿ�"
        << std::setw(6) << "Ψһ" << std::endl;
    std::cout << std::string(51, '-') << std::endl;

    for (const auto& col : table.columns) {
        std::cout << std::left << std::setw(15) << col.column_name
            << std::setw(10) << col.data_type
            << std::setw(8) << col.max_length
            << std::setw(6) << (col.is_primary_key ? "��" : "��")
            << std::setw(6) << (col.is_nullable ? "��" : "��")
            << std::setw(6) << (col.is_unique ? "��" : "��") << std::endl;
    }
}

void CatalogManager::ShowIndexList(const std::string& db_name) {
    auto indexes = ListIndexes(db_name);

    std::cout << "\n=== ���ݿ� " << db_name << " �����б� ===" << std::endl;
    std::cout << std::left << std::setw(20) << "������"
        << std::setw(15) << "����"
        << std::setw(8) << "Ψһ"
        << std::setw(20) << "����" << std::endl;
    std::cout << std::string(63, '-') << std::endl;

    for (const auto& index : indexes) {
        std::string columns;
        for (size_t i = 0; i < index.column_names.size(); i++) {
            if (i > 0) columns += ", ";
            columns += index.column_names[i];
        }

        std::cout << std::left << std::setw(20) << index.index_name
            << std::setw(15) << index.table_name
            << std::setw(8) << (index.is_unique ? "��" : "��")
            << std::setw(20) << columns << std::endl;
    }
}

void CatalogManager::ShowStatistics(const std::string& db_name) {
    auto stats = GetDatabaseStatistics(db_name);

    std::cout << "\n=== ���ݿ� " << db_name << " ͳ����Ϣ ===" << std::endl;
    std::cout << "��ҳ����: " << stats.total_pages << std::endl;
    std::cout << "����ҳ����: " << stats.free_pages << std::endl;
    std::cout << "���ݿ��С: " << stats.total_size << " bytes" << std::endl;
    std::cout << "��������: " << stats.transaction_count << std::endl;
    std::cout << "�ܶ�ȡ����: " << stats.total_reads << std::endl;
    std::cout << "��д�����: " << stats.total_writes << std::endl;
    std::cout << "��������: " << stats.cache_hits << std::endl;
    std::cout << "����δ����: " << stats.cache_misses << std::endl;
    std::cout << "��ǰ������: " << stats.active_connections << std::endl;
    std::cout << "����������: " << std::fixed << std::setprecision(2) << stats.cache_hit_ratio << "%" << std::endl;
    std::cout << "�洢������: " << std::fixed << std::setprecision(2) << stats.storage_utilization << "%" << std::endl;
}

void CatalogManager::GenerateSystemReport() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                    HAODB ϵͳ����" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    ShowSystemInfo();
    ShowDatabaseList();

    auto databases = ListDatabases();
    for (const auto& db : databases) {
        ShowTableList(db.db_name);
        ShowStatistics(db.db_name);
    }
}

void CatalogManager::GenerateDatabaseReport(const std::string& db_name) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                ���ݿ� " << db_name << " ��ϸ����" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    ShowTableList(db_name);
    ShowIndexList(db_name);
    ShowStatistics(db_name);

    if (LoadDatabaseManager(db_name)) {
        auto& db_manager = database_managers[db_name];
        db_manager->GeneratePerformanceReport();
    }
}

// ==================== ���ù��� ====================

bool CatalogManager::SetMaxDatabases(uint32_t max_db) {
    system_config->SetMaxDatabases(max_db);
    return system_config->SaveConfig();
}

bool CatalogManager::SetMaxConnections(uint32_t max_connections) {
    system_config->SetMaxConnections(max_connections);
    return system_config->SaveConfig();
}

bool CatalogManager::SetGlobalBufferSize(uint32_t buffer_size) {
    system_config->SetGlobalBufferSize(buffer_size);
    return system_config->SaveConfig();
}

bool CatalogManager::EnableQueryCache(bool enable) {
    system_config->EnableQueryCache(enable);
    return system_config->SaveConfig();
}

bool CatalogManager::EnableAuthentication(bool enable) {
    system_config->EnableAuthentication(enable);
    return system_config->SaveConfig();
}

const SystemConfig& CatalogManager::GetSystemConfig() {
    return system_config->GetConfig();
}

// ==================== �������� ====================

bool CatalogManager::CreateDirectoryStructure(const std::string& db_name) {
    try {
        std::string db_path = haodb_root_path + "\\" + db_name;

        // �������ݿ��Ŀ¼
        fs::create_directories(db_path);

        // ������Ŀ¼
        fs::create_directories(db_path + "\\data");
        fs::create_directories(db_path + "\\index");
        fs::create_directories(db_path + "\\logs");

        std::cout << "���ݿ�Ŀ¼�ṹ�����ɹ�: " << db_path << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "����Ŀ¼�ṹʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::RemoveDirectoryStructure(const std::string& db_name) {
    try {
        std::string db_path = haodb_root_path + "\\" + db_name;

        if (fs::exists(db_path)) {
            fs::remove_all(db_path);
            std::cout << "���ݿ�Ŀ¼ɾ���ɹ�: " << db_path << std::endl;
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "ɾ��Ŀ¼�ṹʧ��: " << e.what() << std::endl;
        return false;
    }
}

uint32_t CatalogManager::GenerateNewDatabaseId() {
    const auto& databases = registry_manager->GetDatabases();
    uint32_t max_id = 0;

    for (const auto& db : databases) {
        if (db.db_id > max_id) {
            max_id = db.db_id;
        }
    }

    return max_id + 1;
}

uint32_t CatalogManager::GenerateNewTableId(const std::string& db_name) {
    if (!LoadDatabaseManager(db_name)) {
        return 1;
    }

    auto& db_manager = database_managers[db_name];
    const auto& tables = db_manager->GetTables();
    uint32_t max_id = 0;

    for (const auto& table : tables) {
        if (table.table_id > max_id) {
            max_id = table.table_id;
        }
    }

    return max_id + 1;
}

uint32_t CatalogManager::GenerateNewIndexId(const std::string& db_name) {
    if (!LoadDatabaseManager(db_name)) {
        return 1;
    }

    auto& db_manager = database_managers[db_name];
    const auto& indexes = db_manager->GetIndexes();
    uint32_t max_id = 0;

    for (const auto& index : indexes) {
        if (index.index_id > max_id) {
            max_id = index.index_id;
        }
    }

    return max_id + 1;
}

uint8_t CatalogManager::ParseDataType(const std::string& type_str) {
    std::string type = type_str;
    std::transform(type.begin(), type.end(), type.begin(), ::toupper);

    if (type == "INT" || type == "INTEGER") return 1;
    if (type == "VARCHAR" || type == "STRING") return 2;
    if (type == "DECIMAL" || type == "FLOAT") return 3;
    if (type == "DATETIME" || type == "DATE") return 4;

    return 2; // Ĭ��ΪVARCHAR
}

std::string CatalogManager::DataTypeToString(uint8_t type) {
    switch (type) {
    case 1: return "INT";
    case 2: return "VARCHAR";
    case 3: return "DECIMAL";
    case 4: return "DATETIME";
    default: return "UNKNOWN";
    }
}

bool CatalogManager::LoadDatabaseManager(const std::string& db_name) {
    // ����Ѿ����أ�ֱ�ӷ���
    if (database_managers.find(db_name) != database_managers.end()) {
        return true;
    }

    // ������ݿ��Ƿ����
    if (!DatabaseExists(db_name)) {
        return false;
    }

    try {
        std::string metadata_file = haodb_root_path + "\\" + db_name + "\\meta_data.hao";
        auto db_manager = std::make_unique<DatabaseMetadataManager>(metadata_file);

        if (!db_manager->LoadMetadata()) {
            return false;
        }

        database_managers[db_name] = std::move(db_manager);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "�������ݿ������ʧ��: " << e.what() << std::endl;
        return false;
    }
}

bool CatalogManager::ValidateColumnDefinitions(const std::vector<ColumnDefinition>& columns) {
    if (columns.empty()) {
        std::cerr << "�ж��岻��Ϊ��" << std::endl;
        return false;
    }

    if (columns.size() > 32) {
        std::cerr << "���������ܳ���32" << std::endl;
        return false;
    }

    for (const auto& col : columns) {
        if (!ValidateColumnName(col.column_name)) {
            return false;
        }
    }

    return true;
}

bool CatalogManager::ValidateDatabaseName(const std::string& db_name) {
    if (db_name.empty() || db_name.length() > 63) {
        return false;
    }

    // ����Ƿ�����Ƿ��ַ�
    for (char c : db_name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

bool CatalogManager::ValidateTableName(const std::string& table_name) {
    if (table_name.empty() || table_name.length() > 63) {
        return false;
    }

    for (char c : table_name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

bool CatalogManager::ValidateColumnName(const std::string& column_name) {
    if (column_name.empty() || column_name.length() > 63) {
        return false;
    }

    for (char c : column_name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

bool CatalogManager::UpdateColumnOffset(std::vector<ColumnMeta>& columns) {
    for (int i = 0; i < columns.size();i++) {
        if (i == 0)
            columns[i].column_offset = 0;
        else
            columns[i].column_offset = columns[i - 1].column_offset + columns[i - 1].max_length;
    }

    return true;
}