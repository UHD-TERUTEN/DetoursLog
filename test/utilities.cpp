#include "pch.h"
#include "../src/utilities.h"
#include "../src/utilities.cpp"
#include "../src/FileAccessInfo.h"

#include <filesystem>
using namespace std::filesystem;

#include <regex>
#include <fstream>
using namespace std::literals::string_literals;

#include <stdlib.h>
#include <winternl.h>

#include <nlohmann/json.hpp>
#include <detours.h>

using PWriteFile = BOOL(WINAPI*)(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    );
using PNativeWriteFile = NTSTATUS(*)(
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
PWriteFile TrueWriteFile = WriteFile;
PNativeWriteFile TrueNtWriteFile = (PNativeWriteFile)DetourFindFunction("ntdll.dll", "NtWriteFile");
bool hasNtdll = false;

TEST(UtilitiesTest, CanExecute)
{
  FileInfo fileInfo{};
  fileInfo.fileName = R"(DetoursLog.dll)";

  EXPECT_TRUE(IsExecutable(fileInfo));
}

TEST(UtilitiesTest, ConvertUtf8AndWstring)
{
  char* bytes = "팔레트 (Feat. G-DRAGON)";

  auto wstr = ToWstring(bytes);
  auto utf8 = ToUtf8String(wstr.c_str(), wstr.size());

  EXPECT_STREQ(utf8.c_str(), bytes);
}

TEST(UtilitiesTest, LogToTempFile)
{
  InitLogger();
  
  nlohmann::json json({});
  Log(json);
  LogException(std::exception("test"));
  
  auto filename = GetLogDirectoryName() + GetShortProgramName() + ".txt"s;
  std::ifstream log(filename);
  std::string text{};
  
  std::getline(log, text);
  EXPECT_STREQ(text.c_str(), "{}");

  std::getline(log, text);
  EXPECT_STREQ(text.c_str(), R"({"error occurred":["reason","test"]})");

  log.close();
  ASSERT_TRUE(CloseHandle(logger));
  ASSERT_EQ(0, std::remove(filename.c_str()));
}

TEST(UtilitiesTest, GetJsonFileAccessInfo)
{
  FileAccessInfo fileAccessInfo = {"functionName", true, 1};
  nlohmann::json json({});
  json["fileAccessInfo"] = GetJson(fileAccessInfo);

  EXPECT_STREQ(json["fileAccessInfo"]["functionName"].get<std::string>().c_str(), "functionName");
  EXPECT_EQ(json["fileAccessInfo"]["returnValue"], 1);
  EXPECT_EQ(json["fileAccessInfo"]["errorCode"], 1);
}

TEST(UtilitiesTest, ExtractFileExtension)
{
  EXPECT_STREQ(GetFileExtension("utilities.cpp").c_str(),	".cpp");
  EXPECT_STREQ(GetFileExtension("test.exe.txt").c_str(),	".txt");
  EXPECT_STREQ(GetFileExtension(".gitignore").c_str(),		".gitignore");
}

TEST(UtilitiesTest, ValidProgramNames)
{
  std::string currentProgramName = absolute(R"(.\test.exe)").string();
  std::string shorten = R"(\test.exe)";

  auto wProgramName = GetCurrentProgramName();
  std::string programName = ToUtf8String(wProgramName, std::wcslen(wProgramName));

  EXPECT_STREQ(currentProgramName.c_str(), programName.c_str());
  EXPECT_STREQ(shorten.c_str(), GetShortProgramName().c_str());
}
