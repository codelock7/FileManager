#pragma once
#include <QString>
#include <string_view>
#include <filesystem>


namespace fs = std::filesystem;


QString operator/(const QString& lhs, const QString& rhs);
std::wstring operator/(std::wstring_view lhs, std::wstring_view rhs);
std::wstring gluePath(std::wstring_view lhs, std::wstring_view rhs, wchar_t sep = fs::path::preferred_separator);
std::wstring_view getFileName(std::wstring_view, wchar_t sep = fs::path::preferred_separator);
QString toQString(std::wstring_view);
QString toQString(const std::string&);
std::wstring_view toWStringView(const QString&);
