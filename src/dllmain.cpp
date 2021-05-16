#define WIN32_LEAN_AND_MEAN
#include "FileInfo.h"
#include "ModuleInfo.h"
#include "FileAccessInfo.h"
using namespace LogData;

#include "utilities.h"

#include "Logger.h"

#include <string>
#include <sstream>

#include <detours.h>
#include <winternl.h>

using PCreateFileA = HANDLE (WINAPI*)(
    LPCSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
);
using PCreateFileW = HANDLE (WINAPI*)(
    LPCWSTR               lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
);
using PReadFile = BOOL (WINAPI*)(
    HANDLE       hFile,
    LPVOID       lpBuffer,
    DWORD        nNumberOfBytesToRead,
    LPDWORD      lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
);
using PReadFileEx = BOOL (WINAPI*)(
    HANDLE                          hFile,
    LPVOID                          lpBuffer,
    DWORD                           nNumberOfBytesToRead,
    LPOVERLAPPED                    lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
using PWriteFile = BOOL (WINAPI*)(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
);
using PWriteFileEx = BOOL (WINAPI*)(
    HANDLE                          hFile,
    LPCVOID                         lpBuffer,
    DWORD                           nNumberOfBytesToWrite,
    LPOVERLAPPED                    lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
using PNativeCreateFile = NTSTATUS (*)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
);
using PNativeOpenFile = NTSTATUS (*)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
);
using PNativeReadFile = NTSTATUS(*)(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
);
using PNativeWriteFile = NTSTATUS (*)(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
);

// Windows APIs
//
       PCreateFileA TrueCreateFileA = CreateFileA;
static PCreateFileW TrueCreateFileW = CreateFileW;
static PReadFile TrueReadFile       = ReadFile;
static PReadFileEx TrueReadFileEx   = ReadFileEx;
       PWriteFile TrueWriteFile     = WriteFile;
static PWriteFileEx TrueWriteFileEx = WriteFileEx;

// NT functions
//
static PNativeCreateFile TrueNtCreateFile   = (PNativeCreateFile)DetourFindFunction("ntdll.dll", "NtCreateFile");
static PNativeOpenFile TrueNtOpenFile       = (PNativeOpenFile)DetourFindFunction("ntdll.dll", "NtOpenFile");
static PNativeReadFile TrueNtReadFile       = (PNativeReadFile)DetourFindFunction("ntdll.dll", "NtReadFile");
       PNativeWriteFile TrueNtWriteFile     = (PNativeWriteFile)DetourFindFunction("ntdll.dll", "NtWriteFile");

// ZW functions
//
static PNativeCreateFile TrueZwCreateFile   = (PNativeCreateFile)DetourFindFunction("ntdll.dll", "ZwCreateFile");
static PNativeOpenFile TrueZwOpenFile       = (PNativeOpenFile)DetourFindFunction("ntdll.dll", "ZwOpenFile");
static PNativeReadFile TrueZwReadFile       = (PNativeReadFile)DetourFindFunction("ntdll.dll", "ZwReadFile");
static PNativeWriteFile TrueZwWriteFile     = (PNativeWriteFile)DetourFindFunction("ntdll.dll", "ZwWriteFile");

HANDLE logger{};

static std::string prevFunction{};
static std::string prevFileName{};
static HANDLE prevFileHandle = NULL;

static void WriteLog(HANDLE FileHandle, const char* functionName, int ret)
{
    try
    {
        //nlohmann::json json({ {"ProgramName", GetCurrentProgramName()} });
        nlohmann::json json({});
        {
            auto fileAccessInfo = MakeFileAccessInfo(functionName, ret);
            json["fileAccessInfo"] = GetJson(fileAccessInfo);
        }
        {
            auto fileInfo = MakeFileInfo(FileHandle);
            json["fileInfo"] = GetJson(fileInfo);

            if (IsExecutable(fileInfo))
            {
                auto moduleInfo = MakeModuleInfo(fileInfo.fileName);
                json["moduleInfo"] = GetJson(moduleInfo);
            }
        }
        Log(json);
    }
    catch (std::exception& e)
    {
        LogException(e);
    }
}

static void WriteLog(std::string fileName, const char* functionName, NTSTATUS ret)
{
    try
    {
        nlohmann::json json({});
        {
            auto fileAccessInfo = MakeFileAccessInfo(functionName, ret);
            json["fileAccessInfo"] = GetJson(fileAccessInfo);
        }
        {
            auto fileInfo = MakeFileInfo(fileName);
            json["fileInfo"] = GetJson(fileInfo);
        }
        Log(json);
    }
    catch (std::exception& e)
    {
        LogException(e);
    }
}

// Windows APIs
//
__declspec(dllexport)
HANDLE WINAPI DetouredCreateFileA(
    LPCSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
)
{
    auto ret = TrueCreateFileA(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );
    if (prevFunction != __FUNCTION__ && prevFileName != lpFileName)
    {
        WriteLog(lpFileName, __FUNCTION__, ret != NULL);
        prevFunction = __FUNCTION__;
        prevFileName = lpFileName;
    }
    //WriteLog(lpFileName, __FUNCTION__, ret != NULL);
    return ret;
}

HANDLE WINAPI DetouredCreateFileW(
    LPCWSTR               lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
)
{
    auto ret = TrueCreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );
    auto fileName = ToUtf8String(lpFileName, wcslen(lpFileName));
    if (prevFunction != __FUNCTION__ && prevFileName != fileName)
    {
        WriteLog(fileName, __FUNCTION__, ret != NULL);
        prevFunction = __FUNCTION__;
        prevFileName = fileName;
    }
    //WriteLog(fileName, __FUNCTION__, ret != NULL);
    return ret;
}

BOOL WINAPI DetouredReadFile(
    HANDLE       hFile,
    LPVOID       lpBuffer,
    DWORD        nNumberOfBytesToRead,
    LPDWORD      lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
)
{
    auto ret = TrueReadFile(
        hFile,
        lpBuffer,
        nNumberOfBytesToRead,
        lpNumberOfBytesRead,
        lpOverlapped
    );
    if (prevFunction != __FUNCTION__ && prevFileHandle != hFile)
    {
        WriteLog(hFile, __FUNCTION__, ret);
        prevFunction = __FUNCTION__;
        prevFileHandle = hFile;
    }
    //WriteLog(hFile, __FUNCTION__, ret);
    return ret;
}

BOOL WINAPI DetouredReadFileEx(
    HANDLE                          hFile,
    LPVOID                          lpBuffer,
    DWORD                           nNumberOfBytesToRead,
    LPOVERLAPPED                    lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    auto ret = TrueReadFileEx(
        hFile,
        lpBuffer,
        nNumberOfBytesToRead,
        lpOverlapped,
        lpCompletionRoutine
    );
    if (prevFunction != __FUNCTION__ && prevFileHandle != hFile)
    {
        WriteLog(hFile, __FUNCTION__, ret);
        prevFunction = __FUNCTION__;
        prevFileHandle = hFile;
    }
    //WriteLog(hFile, __FUNCTION__, ret);
    return ret;
}

BOOL WINAPI DetouredWriteFile(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
)
{
    auto ret = TrueWriteFile(
        hFile,
        lpBuffer,
        nNumberOfBytesToWrite,
        lpNumberOfBytesWritten,
        lpOverlapped
    );
    if (prevFunction != __FUNCTION__ && prevFileHandle != hFile)
    {
        WriteLog(hFile, __FUNCTION__, ret);
        prevFunction = __FUNCTION__;
        prevFileHandle = hFile;
    }
    //WriteLog(hFile, __FUNCTION__, ret);
    return ret;
}

BOOL WINAPI DetouredWriteFileEx(
    HANDLE                          hFile,
    LPCVOID                         lpBuffer,
    DWORD                           nNumberOfBytesToWrite,
    LPOVERLAPPED                    lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    auto ret = TrueWriteFileEx(
        hFile,
        lpBuffer,
        nNumberOfBytesToWrite,
        lpOverlapped,
        lpCompletionRoutine
    );
    if (prevFunction != __FUNCTION__ && prevFileHandle != hFile)
    {
        WriteLog(hFile, __FUNCTION__, ret);
        prevFunction = __FUNCTION__;
        prevFileHandle = hFile;
    }
    //WriteLog(hFile, __FUNCTION__, ret);
    return ret;
}

// NT functions
//
__declspec(dllexport)
NTSTATUS DetouredNtCreateFile
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
)
{
    auto ret = TrueNtCreateFile
    (
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredNtOpenFile
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
)
{
    auto ret = TrueNtOpenFile
    (
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        ShareAccess,
        OpenOptions
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredNtReadFile
(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
)
{
    auto ret = TrueNtReadFile
    (
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredNtWriteFile
(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
)
{
    auto ret = TrueNtWriteFile
    (
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}


// ZW functions
//
__declspec(dllexport)
NTSTATUS DetouredZwCreateFile
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
)
{
    auto ret = TrueZwCreateFile
    (
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredZwOpenFile
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
)
{
    auto ret = TrueZwOpenFile
    (
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        ShareAccess,
        OpenOptions
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredZwReadFile
(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
)
{
    auto ret = TrueZwReadFile
    (
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

__declspec(dllexport)
NTSTATUS DetouredZwWriteFile
(
    HANDLE           FileHandle,
    HANDLE           Event,
    PIO_APC_ROUTINE  ApcRoutine,
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
)
{
    auto ret = TrueZwWriteFile
    (
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
    );
    WriteLog(FileHandle, __FUNCTION__, ret);
    return ret;
}

static bool HasNtdll()
{
    static const char* ntdll = "ntdll.dll";

    for (HMODULE hModule = NULL; (hModule = DetourEnumerateModules(hModule)) != NULL;)
    {
        char moduleName[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(hModule, moduleName, sizeof(moduleName) - 1)
            && !std::strcmp(moduleName, ntdll))
        {
            return true;
        }
    }
    return false;
}

bool hasNtdll = HasNtdll();

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    InitLogger();

    DetourRestoreAfterWith();

    DisableThreadLibraryCalls(hModule);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (hasNtdll)
    {
        DetourAttach(&(PVOID&)TrueNtCreateFile, DetouredNtCreateFile);
        DetourAttach(&(PVOID&)TrueNtOpenFile, DetouredNtOpenFile);
        DetourAttach(&(PVOID&)TrueNtReadFile, DetouredNtReadFile);
        DetourAttach(&(PVOID&)TrueNtWriteFile, DetouredNtWriteFile);

        DetourAttach(&(PVOID&)TrueZwCreateFile, DetouredZwCreateFile);
        DetourAttach(&(PVOID&)TrueZwOpenFile, DetouredZwOpenFile);
        DetourAttach(&(PVOID&)TrueZwReadFile, DetouredZwReadFile);
        DetourAttach(&(PVOID&)TrueZwWriteFile, DetouredZwWriteFile);
    }
    else
    {
        DetourAttach(&(PVOID&)TrueCreateFileA, DetouredCreateFileA);
        DetourAttach(&(PVOID&)TrueCreateFileW, DetouredCreateFileW);
        DetourAttach(&(PVOID&)TrueReadFile, DetouredReadFile);
        DetourAttach(&(PVOID&)TrueReadFileEx, DetouredReadFileEx);
        DetourAttach(&(PVOID&)TrueWriteFile, DetouredWriteFile);
        DetourAttach(&(PVOID&)TrueWriteFileEx, DetouredWriteFileEx);
    }
    DetourTransactionCommit();
}

void WINAPI ProcessDetach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (hasNtdll)
    {
        DetourDetach(&(PVOID&)TrueNtCreateFile, DetouredNtCreateFile);
        DetourDetach(&(PVOID&)TrueNtOpenFile, DetouredNtOpenFile);
        DetourDetach(&(PVOID&)TrueNtReadFile, DetouredNtReadFile);
        DetourDetach(&(PVOID&)TrueNtWriteFile, DetouredNtWriteFile);

        DetourDetach(&(PVOID&)TrueZwCreateFile, DetouredZwCreateFile);
        DetourDetach(&(PVOID&)TrueZwOpenFile, DetouredZwOpenFile);
        DetourDetach(&(PVOID&)TrueZwReadFile, DetouredZwReadFile);
        DetourDetach(&(PVOID&)TrueZwWriteFile, DetouredZwWriteFile);
    }
    else
    {
        DetourDetach(&(PVOID&)TrueCreateFileA, DetouredCreateFileA);
        DetourDetach(&(PVOID&)TrueCreateFileW, DetouredCreateFileW);
        DetourDetach(&(PVOID&)TrueReadFile, DetouredReadFile);
        DetourDetach(&(PVOID&)TrueReadFileEx, DetouredReadFileEx);
        DetourDetach(&(PVOID&)TrueWriteFile, DetouredWriteFile);
        DetourDetach(&(PVOID&)TrueWriteFileEx, DetouredWriteFileEx);
    }
    DetourTransactionCommit();
}


#include <fstream>

BOOL APIENTRY DllMain( HMODULE  hModule,
                       DWORD    ul_reason_for_call,
                       LPVOID   lpReserved)
{
    if (DetourIsHelperProcess())
        return TRUE;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ProcessAttach(hModule, ul_reason_for_call, lpReserved);
        break;
    case DLL_PROCESS_DETACH:
        ProcessDetach(hModule, ul_reason_for_call, lpReserved);
        break;
    }
    return TRUE;
}
