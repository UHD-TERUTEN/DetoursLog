#include "FileInformation.h"
#include <string>

inline
bool IsDll(const FileInformation& fileInfo)
{
    return fileInfo.fileName.rfind(".dll") != std::string::npos
        || fileInfo.fileName.rfind(".DLL") != std::string::npos;
}

inline
std::wstring ToWstring(const std::string& s)
{
    std::wstring res(std::begin(s), std::end(s));
    return res;
}

inline
std::string ToString(const std::wstring& s)
{
    std::string res(std::begin(s), std::end(s));
    return res;
}
