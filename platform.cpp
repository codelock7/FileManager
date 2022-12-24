#include "platform.h"

#include <Windows.h>
#include <shellapi.h>
#include <WinUser.h>


void Platform::open(const char* path)
{
    ShellExecute(nullptr, nullptr, path, nullptr, nullptr, SW_SHOWNORMAL);
}
