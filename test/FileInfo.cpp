#include "pch.h"
#include "../src/FileInfo.h"
#include "../src/FIleInfo.cpp"
using namespace LogData;

#include "../src/utilities.h"

#include <filesystem>
using namespace std::filesystem;

TEST(FileInfoTest, MakeFileInfoOfThisFile)
{
    auto file = CreateFileA(R"(..\..\FileInfo.cpp)",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    ASSERT_NE(file, INVALID_HANDLE_VALUE);

    auto fileInfo = MakeFileInfo(file);
    uint64_t creationTime = 132646974896520752ULL;

    EXPECT_STREQ(fileInfo.fileName.c_str(), absolute(R"(..\..\FileInfo.cpp)").string().c_str());
    EXPECT_EQ(fileInfo.fileSize, 812);
    EXPECT_EQ(fileInfo.creationTime, creationTime);
    EXPECT_FALSE(fileInfo.isHidden);
}
