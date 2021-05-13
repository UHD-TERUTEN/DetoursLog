#include "pch.h"
#include "../src/Logger.h"
#include "../src/utilities.h"

#include <fstream>

HANDLE logger{};

TEST(LoggerTest, LogTest)
{
  InitLogger();
  Log({ { "foo", "bar" } });

  std::string loggerRoot = std::getenv("LogGathererRoot");
  auto filename = loggerRoot + R"(\Logs\test.exe.txt)";

  std::ifstream temp(filename);
  char buffer[14]{};
  temp.read(buffer, 14);

  EXPECT_STREQ("{\"foo\":\"bar\"}\n", buffer);

  temp.close();
  ASSERT_TRUE(CloseHandle(logger));
  ASSERT_EQ(0, std::remove(filename.c_str()));
}
