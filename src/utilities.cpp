#include "utilities.h"


bool IsDll(const FileInfo& fileInfo)
{
    return fileInfo.fileName.rfind(".dll") != std::string::npos
        || fileInfo.fileName.rfind(".DLL") != std::string::npos;
}

std::wstring ToWstring(const std::string& s)
{
    std::wstring res(std::begin(s), std::end(s));
    return res;
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


// https://stackoverflow.com/questions/2290154/multiple-dlls-writing-to-the-same-text-file
struct Mutex
{
    Mutex() { handle = CreateMutex(0, false, L"logger_mutex"); }
    ~Mutex() { CloseHandle(handle); }

    HANDLE handle;
};

class MutexLock
{
public:
    explicit MutexLock(Mutex& m)
        : m(m)
    {
        WaitForSingleObject(m.handle, INFINITE);
    }
    ~MutexLock()
    {
        ReleaseMutex(m.handle);
    }

private:
    Mutex& m;
};

static Mutex mutex{};


void Log(const nlohmann::json& json)
{
    MutexLock lock(mutex);
    logger << json << std::endl;
}

void LogException(const std::exception& e)
{
    Log({ { "error occurred", { "reason", e.what() } } });
}
