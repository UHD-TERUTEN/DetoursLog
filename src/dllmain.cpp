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

using PCreateFile = NTSTATUS (*)(
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
using POpenFile = NTSTATUS (*)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
);
using PReadFile = NTSTATUS(*)(
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
using PWriteFile = NTSTATUS (*)(
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

// NT functions
//
static PCreateFile TrueNtCreateFile = (PCreateFile)DetourFindFunction("ntdll.dll", "NtCreateFile");
static POpenFile TrueNtOpenFile = (POpenFile)DetourFindFunction("ntdll.dll", "NtOpenFile");
static PReadFile TrueNtReadFile = (PReadFile)DetourFindFunction("ntdll.dll", "NtReadFile");
static PWriteFile TrueNtWriteFile = (PWriteFile)DetourFindFunction("ntdll.dll", "NtWriteFile");

// ZW functions
//
static PCreateFile TrueZwCreateFile = (PCreateFile)DetourFindFunction("ntdll.dll", "ZwCreateFile");
static POpenFile TrueZwOpenFile = (POpenFile)DetourFindFunction("ntdll.dll", "ZwOpenFile");
static PReadFile TrueZwReadFile = (PReadFile)DetourFindFunction("ntdll.dll", "ZwReadFile");
static PWriteFile TrueZwWriteFile = (PWriteFile)DetourFindFunction("ntdll.dll", "ZwWriteFile");

std::ofstream logger{};

static void WriteLog(HANDLE FileHandle, const char* functionName, NTSTATUS ret)
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

        if (logger.bad())
        {
            logger.clear();
            Log({ { "state", logger.rdstate() } });
        }
    }
    catch (std::exception& e)
    {
        LogException(e);
    }
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

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    InitLogger();
    Log({ { "ProcessAttach", "Init" } });

    DetourRestoreAfterWith();

    DisableThreadLibraryCalls(hModule);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)TrueNtCreateFile, DetouredNtCreateFile);
    DetourAttach(&(PVOID&)TrueNtOpenFile, DetouredNtOpenFile);
    DetourAttach(&(PVOID&)TrueNtReadFile, DetouredNtReadFile);
    DetourAttach(&(PVOID&)TrueNtWriteFile, DetouredNtWriteFile);

    DetourAttach(&(PVOID&)TrueZwCreateFile, DetouredZwCreateFile);
    DetourAttach(&(PVOID&)TrueZwOpenFile, DetouredZwOpenFile);
    DetourAttach(&(PVOID&)TrueZwReadFile, DetouredZwReadFile);
    DetourAttach(&(PVOID&)TrueZwWriteFile, DetouredZwWriteFile);

    DetourTransactionCommit();
}

void WINAPI ProcessDetach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourDetach(&(PVOID&)TrueNtCreateFile, DetouredNtCreateFile);
    DetourDetach(&(PVOID&)TrueNtOpenFile, DetouredNtOpenFile);
    DetourDetach(&(PVOID&)TrueNtReadFile, DetouredNtReadFile);
    DetourDetach(&(PVOID&)TrueNtWriteFile, DetouredNtWriteFile);

    DetourDetach(&(PVOID&)TrueZwCreateFile, DetouredZwCreateFile);
    DetourDetach(&(PVOID&)TrueZwOpenFile, DetouredZwOpenFile);
    DetourDetach(&(PVOID&)TrueZwReadFile, DetouredZwReadFile);
    DetourDetach(&(PVOID&)TrueZwWriteFile, DetouredZwWriteFile);

    DetourTransactionCommit();
    Log({ { "ProcessDetach", "Destroy" } });
}


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
