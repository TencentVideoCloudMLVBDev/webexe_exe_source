#pragma once
#include <string>
#include <windows.h>

namespace LiteAvActiveX
{
	UINT_PTR OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
	std::wstring screenShotDefaultName();
}
