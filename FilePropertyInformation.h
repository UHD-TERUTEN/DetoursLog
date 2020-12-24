#pragma once
#include <string>

struct FilePropertyInformation
{
    std::string applicationName = "(EMPTY)";
    std::string fileVersion = "(EMPTY)";
    std::string companyName = "(EMPTY)";
};

FilePropertyInformation GetFilePropertyInformation(std::wstring path);

void LogFilePropertyInformation(const FilePropertyInformation& fileInfo);
