#include "pch.h"
#include "../src/utilities.h"
#include "../src/utilities.cpp"
#include "../src/FileAccessInfo.h"
#include <regex>
#include <filesystem>
#include <fstream>
#include <stdlib.h>
using namespace std::filesystem;

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

  std::ifstream log(GetLogDirectoryName() + GetShortProgramName() + ".txt"s);
  std::string text{};
  
  std::getline(log, text);
  EXPECT_STREQ(text.c_str(), "{}");

  std::getline(log, text);
  EXPECT_STREQ(text.c_str(), R"({"error occurred":["reason","test"]})");

  log.close();
  logger.close();
  (void)remove_all(GetLogDirectoryName());
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
  std::string programName = absolute(R"(.\test.exe)").string();
  std::string shorten = R"(\test.exe)";

  EXPECT_STREQ(programName.c_str(), GetCurrentProgramName());
  EXPECT_STREQ(shorten.c_str(), GetShortProgramName().c_str());
}

TEST(UtilitiesTest, MakeDirectories)
{
  constexpr wchar_t* alreadyExists = LR"(.\foo)";
  constexpr wchar_t* valid = LR"(.\bar)";

  (void)_wmkdir(alreadyExists);

  EXPECT_TRUE(IsDirectoryExists(alreadyExists));
  EXPECT_TRUE(MakeDirectory(alreadyExists));

  EXPECT_FALSE(IsDirectoryExists(valid));
  EXPECT_TRUE(MakeDirectory(valid));

  (void)remove(alreadyExists);
  (void)remove(valid);
}