#include "filemanager.h"

FileManager::FileManager(const std::string& path)
    : currentPath(path)
{
}

std::wstring_view FileManager::getCurrentPath() const
{
    return currentPath.native();
}

void FileManager::goDown()
{
    fs::path parentPath = currentPath.parent_path();
    if (currentPath == parentPath)
        return;
    currentPath = std::move(parentPath);
}

void FileManager::goToDirectory(std::wstring_view directory)
{
    fs::path newPath = currentPath / directory;
    if (!fs::exists(newPath))
        return;
    fs::directory_entry entry(newPath);
    if (!entry.is_directory())
        return;
    currentPath = std::move(newPath);
}

std::vector<std::wstring_view> FileManager::getInnerDirectories() const
{
    std::vector<std::wstring_view> result;
    iter = fs::directory_iterator(currentPath);
    for (const fs::directory_entry& entry : iter) {
        if (!entry.is_directory())
            continue;
        result.emplace_back(entry.path().native());
    }
    return result;
}
