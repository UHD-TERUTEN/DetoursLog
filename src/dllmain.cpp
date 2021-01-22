#define WIN32_LEAN_AND_MEAN
#include "FileInfo.h"
#include "ModuleInfo.h"
#include "FileAccessInfo.h"
using namespace LogData;

#include "utilities.h"

#include "Logger.h"

#include <string>

#include <detours.h>

using PReadFile = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
static PReadFile TrueReadFile = ReadFile;

std::ofstream logger{};

static char* GetCurrentProgramName()
{
    static char programName[MAX_PATH]{};
    std::memset(programName, 0, MAX_PATH);
    GetModuleFileNameA(NULL, programName, MAX_PATH);
    return programName;
}

__declspec(dllexport)
BOOL WINAPI ReadFileWithLog(HANDLE        hFile,
                            LPVOID        lpBuffer,
                            DWORD         nNumberOfBytesToRead,
                            LPDWORD       lpNumberOfBytesRead,
                            LPOVERLAPPED  lpOverlapped)
{
    auto ret = TrueReadFile
    (
        hFile,
        lpBuffer,
        nNumberOfBytesToRead,
        lpNumberOfBytesRead,
        lpOverlapped
    );

    try
    {
        nlohmann::json json({ {"ProgramName", GetCurrentProgramName()} });
        {
            auto fileAccessInfo = MakeFileAccessInfo(__FUNCTION__, ret);
            json["fileAccessInfo"] = GetJson(fileAccessInfo);
        }
        {
            auto fileInfo = MakeFileInfo(hFile);
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
    return ret;
}

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    logger.open(R"(D:\log.txt)", std::ios_base::app);

    DetourRestoreAfterWith();

    DisableThreadLibraryCalls(hModule);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueReadFile, ReadFileWithLog);
    DetourTransactionCommit();
}

void WINAPI ProcessDetach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueReadFile, ReadFileWithLog);
    DetourTransactionCommit();
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
