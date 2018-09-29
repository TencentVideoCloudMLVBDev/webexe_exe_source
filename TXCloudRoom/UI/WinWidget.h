#ifndef WINWIDGET_H
#define WINWIDGET_H

#include "commonType.h"
#include <QtWidgets/QWidget>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class WinWidget
{
public:
	WinWidget();
	~WinWidget();

public:
	void setMenuInfo(MenuInfo & menuInfo);
	QWidget * getWidget();
	HWND getHwnd();
	std::string getId();
	void setId(const std::string &id)
	{
		m_identifier = id;
	}

private:
	std::string	m_identifier;
	QRect		m_Rect;
	MenuInfo m_menuInfo;
	HWND m_hwnd;

private:
	static void OnRButtonUp(HWND hWnd, LPARAM lParam);
	static HWND createWindow();
	static LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif 