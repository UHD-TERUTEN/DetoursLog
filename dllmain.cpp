#define WIN32_LEAN_AND_MEAN
#include "FileInformation.h"
#include "FileVersionInformation.h"

#include <string>
#include <fstream>
#include <mutex>

#include <detours.h>

#include "utilities.cpp"

using PReadFile = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
static PReadFile TrueReadFile = ReadFile;

extern std::ofstream logger;
extern std::ofstream modules;

static std::mutex loggerMutex;
static std::mutex reportMutex;

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
    const std::lock_guard<std::mutex> loggerLock(loggerMutex);

    char fileName[MAX_PATH]{};
    GetModuleFileNameA(NULL, fileName, MAX_PATH);
    logger << "From: " << fileName << std::endl;

    logger  << __FUNCTION__"("
            << hFile << " , "
            << lpBuffer << " , "
            << nNumberOfBytesToRead << " , "
            << lpNumberOfBytesRead << " , "
            << lpOverlapped
            << ")"
            << std::endl
            << __FUNCTION__"->" << ret << std::endl;

    auto fileInfo = GetFileInformation(hFile);
    Log(fileInfo);

    if (logger.bad())
    {
        const std::lock_guard<std::mutex> reportLock(reportMutex);
        modules << logger.rdstate() << std::endl;
        logger.clear();
    }
    return ret;
}

// from Detours sample (trcapi.cpp)
//
PIMAGE_NT_HEADERS NtHeadersForInstance(HINSTANCE hInst)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hInst;
    __try {
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
            pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return NULL;
        }
        return pNtHeader;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    SetLastError(ERROR_EXE_MARKED_INVALID);

    return NULL;
}

BOOL ProcessEnumerate()
{
    char fileName[MAX_PATH]{};
    GetModuleFileNameA(NULL, fileName, MAX_PATH);

    modules << "######################################################### "
            << fileName << " : Binaries" << std::endl;

    PBYTE pbNext;
    for (PBYTE pbRegion = (PBYTE)0x10000;; pbRegion = pbNext)
    {
        MEMORY_BASIC_INFORMATION mbi;
        ZeroMemory(&mbi, sizeof(mbi));

        if (VirtualQuery((PVOID)pbRegion, &mbi, sizeof(mbi)) <= 0) {
            break;
        }
        pbNext = (PBYTE)mbi.BaseAddress + mbi.RegionSize;

        // Skip free regions, reserver regions, and guard pages.
        //
        if (mbi.State == MEM_FREE || mbi.State == MEM_RESERVE) {
            continue;
        }
        if (mbi.Protect & PAGE_GUARD || mbi.Protect & PAGE_NOCACHE) {
            continue;
        }
        if (mbi.Protect == PAGE_NOACCESS) {
            continue;
        }

        // Skip over regions from the same allocation...
        {
            MEMORY_BASIC_INFORMATION mbiStep;

            while (VirtualQuery((PVOID)pbNext, &mbiStep, sizeof(mbiStep)) > 0)
            {
                if ((PBYTE)mbiStep.AllocationBase != pbRegion) {
                    break;
                }
                pbNext = (PBYTE)mbiStep.BaseAddress + mbiStep.RegionSize;
                mbi.Protect |= mbiStep.Protect;
            }
        }

        WCHAR wzDllName[MAX_PATH];
        PIMAGE_NT_HEADERS pinh = NtHeadersForInstance((HINSTANCE)pbRegion);

        if (pinh && GetModuleFileNameW((HINSTANCE)pbRegion, wzDllName, ARRAYSIZE(wzDllName)))
        {
            std::wstring temp(wzDllName);
            std::string name;
            name.assign(std::begin(temp), std::end(temp));
            modules << "-------------------------------------------------------" << std::endl;
            modules << name << std::endl;
            auto versionInfo = GetFileVersionInformation(name);
            Log(versionInfo);
            modules << "-------------------------------------------------------" << std::endl;
        }
    }
    modules << std::endl;

    return TRUE;
}

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    logger.open(R"(D:\log.txt)", std::ios_base::app);
    modules.open(R"(D:\modules.txt)", std::ios_base::app);
    //out.open(R"(C:\Users\gggg8\Desktop\tmp\log.txt)", std::ios_base::app);

    ProcessEnumerate();

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
