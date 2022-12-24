#include "platform.h"

#include <Windows.h>
#include <shellapi.h>
#include <WinUser.h>


void Platform::open(const wchar_t* path)
{
    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask = SEE_MASK_NOASYNC;
    info.lpVerb = L"open";
    info.lpFile = path;
    info.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&info);
}
