#include "pch.h"
#include "../src/Logger.h"
using namespace std::literals::string_literals;

std::ofstream logger{};

TEST(LoggerTest, LogToTempFile)
{
  logger.open("temp");
  logger << "test";
  logger.close();

  std::ifstream temp("temp");
  char buffer[5]{};
  temp.read(buffer, 5);

  EXPECT_TRUE("test"s == buffer);

  temp.close();
  std::remove("temp");
}
