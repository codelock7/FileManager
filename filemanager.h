#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class FileManager
{
public:
    explicit FileManager(const std::string& path);
    std::wstring_view getCurrentPath() const;
    std::vector<std::wstring_view> getInnerDirectories() const;
    void goDown();
    void goToDirectory(std::wstring_view directory);

private:
    fs::path currentPath;
    mutable fs::directory_iterator iter;
};
