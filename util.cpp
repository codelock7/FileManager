#include "util.h"
#include <QDir>
#include <filesystem>


namespace fs = std::filesystem;


QString operator/(const QString& lhs, const QString& rhs)
{
    QString result;
    constexpr int separatorSize = 1;
    result.reserve(lhs.size() + separatorSize + rhs.size());
    result += lhs;
    result += '/';
    result += rhs;
    return result;
}

std::wstring operator/(std::wstring_view lhs, std::wstring_view rhs)
{
    return gluePath(lhs, rhs, fs::path::preferred_separator);
}

std::wstring_view toWStringView(const QString& value) {
    static_assert (sizeof *value.utf16() == sizeof(wchar_t));
    return {reinterpret_cast<const wchar_t*>(value.utf16()), static_cast<size_t>(value.size())};
}

std::wstring_view getFileName(std::wstring_view path, wchar_t sep)
{
    const size_t lastSepPos = path.rfind(sep);
    if (lastSepPos == std::wstring_view::npos)
        return path;
    return path.substr(lastSepPos + 1);
}

QString toQString(std::wstring_view value)
{
    const size_t valueSize = value.size();
    Q_ASSERT(static_cast<int>(valueSize) == valueSize);
    return QString::fromWCharArray(value.data(), static_cast<int>(valueSize));
}

std::wstring gluePath(std::wstring_view lhs, std::wstring_view rhs, wchar_t sep)
{
    std::wstring result;
    constexpr size_t sepSize = 1;
    result.reserve(lhs.size() + sepSize + rhs.size());
    result.append(lhs.data(), lhs.size());
    result.push_back(sep);
    result.append(rhs.data(), rhs.size());
    return result;
}

QString toQString(const std::string& value)
{
    return QString::fromStdString(value);
}
