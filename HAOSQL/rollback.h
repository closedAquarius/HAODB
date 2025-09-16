#pragma once
#include <ctime>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include "buffer_pool.h"

const uint8_t SIZEOFTIME_T = 8,
SIZEOFUINT16_T = 2,
SIZEOFENUM = 4;

struct Log {
	uint16_t pageId;
	uint16_t slotId;
	uint16_t len;
	int type;

	char* rawData();
	const char* rawData() const;
};


void undo(const Log& log, BufferPoolManager* bpm);
void undos(vector<Log>& logs, BufferPoolManager* bpm);
Log fromRawData(const char* buffer);
void writeLog(int type, int pageId, int slotId);