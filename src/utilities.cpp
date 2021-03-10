#include "utilities.h"

#include <mutex>

static std::string GetFileExtension(const std::string& s)
{
    auto extpos = s.rfind('.');
    if (extpos != std::string::npos)
    {
        auto extension = s.substr(extpos);
        std::transform(std::begin(extension), std::end(extension), std::begin(extension), std::tolower);
        return extension;
    }
    return "";
}

static std::string GetCurrentDateString()
{
    SYSTEMTIME st{};
    TCHAR timeString[11]{};

    GetSystemTime(&st);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, timeString, 11);
    return std::string{ timeString, timeString + 10 };
}

static std::string GetLogDirectoryName(const std::string& logPath)
{
    return logPath + GetCurrentDateString();
}

static char* GetCurrentProgramName()
{
    static char programName[MAX_PATH]{};
    if (!*programName)
        GetModuleFileNameA(NULL, programName, MAX_PATH);
    return programName;
}

static std::string GetShortProgramName()
{
    std::string programName = GetCurrentProgramName();
    auto pos = programName.rfind('\\');
    if (pos == std::string::npos)
        return "";
    return programName.substr(pos);
}

static bool IsDirectoryExists(const std::wstring& directoryName)
{
    auto attr = GetFileAttributes(directoryName.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static bool MakeDirectory(const std::wstring& directoryName)
{
    std::wstring subdirectoryName{};
    size_t pos = directoryName.find('\\') + 1;
    int result = 0;

    while ((pos = directoryName.find('\\', pos)) != std::wstring::npos)
    {
        subdirectoryName = directoryName.substr(0, pos);
        if (!IsDirectoryExists(subdirectoryName))
            (void)_wmkdir(subdirectoryName.c_str());
        ++pos;
    }
    if (!IsDirectoryExists(directoryName))
        result = _wmkdir(directoryName.c_str());
    return (result == 0);
}


bool IsExecutable(const FileInfo& fileInfo)
{
    return GetFileExtension(fileInfo.fileName) == ".exe"
        || GetFileExtension(fileInfo.fileName) == ".dll";
}

std::wstring ToWstring(const std::string& utf8)
{
    std::wstring wstr{};

    do
    {
        if (utf8.empty())
            break;
        //
        // getting required cch.
        //
        int required_cch = ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            utf8.c_str(), static_cast<int>(utf8.size()),
            nullptr, 0
        );
        if (0 == required_cch)
            break;
        //
        // allocate.
        //
        wstr.resize(required_cch);
        //
        // convert.
        //
        if (0 == ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            utf8.c_str(), static_cast<int>(utf8.size()),
            const_cast<wchar_t*>(wstr.c_str()), static_cast<int>(wstr.size())
        ))
        {
            break;
        }
    } while (false);
    return wstr;
}

// https://wendys.tistory.com/84
std::string ToUtf8String(const wchar_t* unicode, const size_t unicode_size)
{
    if ((nullptr == unicode) || (0 == unicode_size))
        return{};

    std::string utf8{};

    // getting required cch
    if (int required_cch = ::WideCharToMultiByte(   CP_UTF8,
                                                    WC_ERR_INVALID_CHARS,
                                                    unicode, static_cast<int>(unicode_size),
                                                    nullptr, 0,
                                                    nullptr, nullptr))
    {
        // allocate
        utf8.resize(required_cch);
    }
    else
        return {};

    // convert
    ::WideCharToMultiByte(  CP_UTF8,
                            WC_ERR_INVALID_CHARS,
                            unicode, static_cast<int>(unicode_size),
                            const_cast<char*>(utf8.c_str()), static_cast<int>(utf8.size()),
                            nullptr, nullptr);
    return utf8;
}

void InitLogger(const std::string& logPath)
{
    auto logDirectoryName = GetLogDirectoryName(logPath);
    MakeDirectory(ToWstring(logDirectoryName));

    logger.open(logDirectoryName + GetShortProgramName() + ".txt"s, std::ios_base::app);
}

std::mutex mutex{};

void Log(const nlohmann::json& json)
{
    const std::lock_guard<std::mutex> lock(mutex);
    logger << json << std::endl;
}

void LogException(const std::exception& e)
{
    Log({ { "error occurred", { "reason", e.what() } } });
}
