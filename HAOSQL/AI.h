#pragma once
#include<iostream>
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

std::string callZhipuAI(const std::string& sql);

std::string CALLAI(const std::string& sql);
