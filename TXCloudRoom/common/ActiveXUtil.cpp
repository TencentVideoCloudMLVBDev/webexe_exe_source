#include "ActiveXUtil.h"
#include <string>
#include <time.h>
using namespace std;
using namespace LiteAvActiveX;
UINT_PTR LiteAvActiveX::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if (uiMsg == WM_SHOWWINDOW)
	{
		RECT rtWnd, sysRect;
		::GetWindowRect(GetParent(hdlg), &rtWnd);
		SystemParametersInfo(SPI_GETWORKAREA, 0, &sysRect, 0);

		::MoveWindow(GetParent(hdlg),
			(sysRect.right - (rtWnd.right - rtWnd.left)) / 2,
			(sysRect.bottom - (rtWnd.bottom - rtWnd.top)) / 2,
			rtWnd.right - rtWnd.left,
			rtWnd.bottom - rtWnd.top,
			true);
	}
	return 0;
}

std::wstring LiteAvActiveX::screenShotDefaultName()
{
	time_t timep;
	time(&timep);
	wchar_t tmp[128];
	wcsftime(tmp, sizeof(tmp), L"½ØÍ¼Î´ÃüÃû%H_%M_%S", localtime(&timep));
	return tmp;
}

