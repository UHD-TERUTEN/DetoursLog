// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#define WIN32_LEAN_AND_MEAN
#include "FileInformation.h"
#include "FileVersionInformation.h"

#include <fstream>
#include <mutex>
#include <string>

#include <detours.h>

#include "utilities.cpp"

using PReadFile = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
static PReadFile TrueReadFile = ReadFile;

extern std::ofstream logger;
extern std::ofstream report;

static std::mutex loggerMutex;
static std::mutex reportMutex;

__declspec(dllexport)
BOOL WINAPI ReadFileWithLog(HANDLE        hFile,
                            LPVOID        lpBuffer,
                            DWORD         nNumberOfBytesToRead,
                            LPDWORD       lpNumberOfBytesRead,
                            LPOVERLAPPED  lpOverlapped)
{
    auto ret = TrueReadFile(
        hFile,
        lpBuffer,
        nNumberOfBytesToRead,
        lpNumberOfBytesRead,
        lpOverlapped
    );

    const std::lock_guard<std::mutex> loggerLock(loggerMutex);

    logger  << __FUNCTION__"("
            << hFile << " , "
            << lpBuffer << " , "
            << nNumberOfBytesToRead << " , "
            << lpNumberOfBytesRead << " , "
            << lpOverlapped
            << ")"
            << std::endl
            << __FUNCTION__"->" << ret << std::endl;

    if (auto fileInfoOrEmpty = GetFileInformation(hFile))
    {
        auto fileInfo = fileInfoOrEmpty.value();
        Log(fileInfo);

        if (IsDll(fileInfo))
        {
            auto fileProp = GetFileVersionInformation(fileInfo.fileName);
            Log(fileProp);
        }
    }
    else
        logger << "error: " << GetLastError() << std::endl;

    if (logger.bad())
    {
        const std::lock_guard<std::mutex> reportLock(reportMutex);
        report << logger.rdstate();
        logger.clear();
    }
    return ret;
}

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    DetourRestoreAfterWith();

    DisableThreadLibraryCalls(hModule);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueReadFile, ReadFileWithLog);
    DetourTransactionCommit();

    logger.open(R"(D:\log.txt)", std::ios_base::app);
    logger.open(R"(D:\report.txt)", std::ios_base::app);
    //out.open(R"(C:\Users\gggg8\Desktop\tmp\log.txt)", std::ios_base::app);
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
