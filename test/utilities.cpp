#include "pch.h"
#include "../src/utilities.h"
#include "../src/utilities.cpp"
#include "../src/FileAccessInfo.h"
#include <regex>
#include <filesystem>

TEST(UtilitiesTest, CanExecute)
{
  FileInfo fileInfo{};
  EXPECT_TRUE(IsExecutable(fileInfo));
}

TEST(UtilitiesTest, ConvertUtf8AndWstring)
{
  char* bytes = (char*)L"팔레트 (Feat. G-DRAGON)";
  auto wstr = ToWstring(bytes);
  auto utf8 = ToUtf8String(wstr.c_str(), 21);

  EXPECT_TRUE(utf8 == bytes);
}

TEST(UtilitiesTest, LogToTempFile)
{
  InitLogger("temp_");
  
  nlohmann::json json({});
  Log(json);
  LogException(std::exception("test"));
}

TEST(UtilitiesTest, GetJsonFileAccessInfo)
{
  FileAccessInfo fileAccessInfo = {"functionName", true, 1};
  nlohmann::json json({});
  json["fileAccessInfo"] = GetJson(fileAccessInfo);
  
  EXPECT_TRUE(json["fileAccessInfo"]["functionName"] == "functionName");
  EXPECT_TRUE(json["fileAccessInfo"]["returnValue"]);
  EXPECT_EQ(json["fileAccessInfo"]["errorCode"], 1);
}

TEST(UtilitiesTest, ExtractFileExtension)
{
  EXPECT_TRUE("utilities.cpp"s == ".cpp");
  EXPECT_TRUE("utilities.d.ts"s == ".ts");
  EXPECT_TRUE(".gitignore"s == ".gitignore");
}

TEST(UtilitiesTest, ValidDateString)
{ 
  auto dateTime = GetCurrentDateString();
  std::regex dateTimeRegex{ "" };

  EXPECT_TRUE(std::regex_match(dateTime, dateTimeRegex));
}

TEST(UtilitiesTest, ValidLogDirectoryNameFormat)
{
  std::string logPath = "./foo";
  std::regex logDirectoryNameRegex{ logPath + "+" };
  auto logDirectoryName = GetLogDirectoryName(logPath);

  EXPECT_TRUE(std::regex_match(logDirectoryName, logDirectoryNameRegex));
}

TEST(UtilitiesTest, ValidProgramNames)
{
  std::string programName = "";
  std::string shorten = "";

  EXPECT_TRUE(programName == GetCurrentProgramName());
  EXPECT_TRUE(shorten == GetShortProgramName());
}

TEST(UtilitiesTest, MakeDirectories)
{
  constexpr wchar_t* alreadyExists = L"./foo";
  constexpr wchar_t* valid = L"./bar";

  (void)_wmkdir(alreadyExists);

  EXPECT_TRUE(IsDirectoryExists(alreadyExists));
  EXPECT_FALSE(MakeDirectory(alreadyExists));

  EXPECT_FALSE(IsDirectoryExists(valid));
  EXPECT_TRUE(MakeDirectory(valid));

  (void)std::filesystem::remove(alreadyExists);
  (void)std::filesystem::remove(valid);
}