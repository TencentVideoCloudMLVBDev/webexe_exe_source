#include "WinWidget.h"
#include <windows.h>
#include "RTCVideoWidget.h"
#include "MultiVideoPanel.h"
#include "LiveVideoWidget.h"
#include "LiveVideoPanel.h"
#include <QtGui/QWindow>

WinWidget::WinWidget()
{

}

WinWidget::~WinWidget()
{
	DestroyWindow(m_hwnd);
}

void WinWidget::setMenuInfo(MenuInfo & menuInfo)
{
	m_menuInfo = menuInfo;
}

QWidget * WinWidget::getWidget()
{
	m_hwnd = createWindow();
	return QWidget::createWindowContainer(QWindow::fromWinId((WId)m_hwnd));
}

HWND WinWidget::getHwnd()
{
	return m_hwnd;
}

std::string WinWidget::getId()
{
	return m_identifier;
}

void WinWidget::OnRButtonUp(HWND hWnd, LPARAM lParam)
{
//	RTCVideoWidget * RTCWidget = (RTCVideoWidget*)GetProp(hWnd, L"RTCVideoWidget");
//	MultiVideoPanel * RTCPanel = (MultiVideoPanel*)GetProp(hWnd, L"MultiVideoPanel");

	LiveVideoWidget * LiveWidget = (LiveVideoWidget*)GetProp(hWnd, L"LiveVideoWidget");
//	LiveVideoPanel * LivePanel = (LiveVideoPanel*)GetProp(hWnd, L"LiveVideoPanel");

	if (!LiveWidget)
	{
		return;
	}
	MenuInfo menuInfo;
	menuInfo = LiveWidget->getMenuInfo();
	

	HMENU hPop = CreatePopupMenu();

	//if (menuInfo.camera)
	//{
	//	AppendMenu(hPop, MF_STRING, 40001, __TEXT("关闭摄像头"));
	//}
	//else
	//{
	//	AppendMenu(hPop, MF_STRING, 40002, __TEXT("开启摄像头"));
	//}
	//if (menuInfo.mic && menuInfo.mainDis)
	//{
	//	AppendMenu(hPop, MF_STRING, 40003, __TEXT("关闭麦克风"));
	//}
	//else
	//{
	//	AppendMenu(hPop, MF_STRING, 40004, __TEXT("开启麦克风"));
	//}

	if (menuInfo.linkMic && !menuInfo.mainDis)
	{
		AppendMenu(hPop, MF_STRING, 40005, __TEXT("取消连麦"));
	}

	POINT pt = { 0 };
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	ClientToScreen(hWnd, &pt);
	int nSelectID = TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
	switch (nSelectID)
	{
	case 40005:
		LiveWidget->on_menu_actLink();
		break;
	default:
		break;
	}
}

HWND WinWidget::createWindow()
{
	static int winCount = 0;
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)wndProc;

	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = ::GetModuleHandle(NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = (LPCTSTR)IDR_MENU_VIDEO;
	winCount++;
	wchar_t lcount[32];
	swprintf_s(lcount, L"%s%d", L"displayWnd", winCount);
	wcex.lpszClassName = lcount;

	const ATOM r = RegisterClassEx(&wcex);

	return ::CreateWindow(
		lcount,
		lcount,
		(WS_OVERLAPPEDWINDOW),
		CW_USEDEFAULT, 0,
		CW_USEDEFAULT, 0,
		nullptr,
		nullptr,
		::GetModuleHandle(NULL),
		nullptr);

}

LRESULT WinWidget::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{

	}
	break;
	case WM_TIMER:
		PostMessage(hWnd, WM_PAINT, 0, 0);//Post WM_PAINT message
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);//Must call BeginPaint & EndPaint on WM_PAINT message to remove WM_PATIN message from message queue 
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_ERASEBKGND:
	{

	}
	break;
	case WM_RBUTTONUP:
		OnRButtonUp(hWnd, lParam);
		break;
	default:

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}