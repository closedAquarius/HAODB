#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "database_registry_manager.h"
#include "system_config_manager.h"
#include "database_metadata_manager.h"

class CatalogManager
{
public:
	void RegisterDatabase();
};