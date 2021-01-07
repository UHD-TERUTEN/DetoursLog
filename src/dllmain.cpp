﻿#define WIN32_LEAN_AND_MEAN
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
        }
        Log(json);

        if (logger.bad())
        {
            Log({ { "state", logger.rdstate() } });
            logger.clear();
        }
    }
    catch (std::exception& e)
    {
        LogException(e);
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

            try
            {
                nlohmann::json json({});
                {
                    auto moduleInfo = MakeModuleInfo(name);
                    json["moduleInfo"] = GetJson(moduleInfo);
                }
                Log(json);
            }
            catch (std::exception& e)
            {
                LogException(e);
            }
        }
    }
    return TRUE;
}

void WINAPI ProcessAttach(  HMODULE hModule,
                            DWORD   ul_reason_for_call,
                            LPVOID  lpReserved)
{
    logger.open(R"(D:\log.txt)", std::ios_base::app);
    //logger.open(R"(C:\Users\gggg8\Desktop\tmp\log.txt)", std::ios_base::app);

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