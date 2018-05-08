#include <QtWidgets/QApplication>
#include "CrashDump.h"
#include "DialogPushPlay.h"
#include "log.h"
#include <windows.h>
#include <Psapi.h>
#include <QDebug>
#include <QProcess>
#include "tlhelp32.h"
#pragma   comment(lib,"psapi.lib")
#define CS_PROCESS_MUTEX_NAME  L"Mutex:{9490BA78-EAC2-438C-880F-5C0C8E30E564}-PROCESS-MUTEX"

int main(int argc, char *argv[])
{
    LOGGER;

	/*
	int result = 0;
	QString processName;

	//提升进程权限
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	int n = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	n = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	CloseHandle(hToken);

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pInfo;
	pInfo.dwSize = sizeof(pInfo);
	Process32First(hSnapShot, &pInfo);

	int count = 0;
	do
	{
		//遍历任务的所有进程
		processName = (QString::fromUtf16(reinterpret_cast<const unsigned short *>(pInfo.szExeFile)));
		if (processName == "CS.exe")
		{
			count++;
		}
	} while (Process32Next(hSnapShot, &pInfo));
	CloseHandle(hSnapShot);
	if (count >= 2)
	{
		return 0;
	}*/

	HANDLE m_hMutex = NULL;
	std::wstring strMutexName = CS_PROCESS_MUTEX_NAME;
	m_hMutex = ::CreateMutex(NULL, FALSE, strMutexName.c_str());
	if (m_hMutex != NULL && ::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		::CloseHandle(m_hMutex);
		m_hMutex = NULL;
		LINFO(L"CS.exe exist");
		return 0;
	}
	LINFO(L"CS.exe not exist");

	CrashDump dump;
	QApplication a(argc, argv);

	DialogPushPlay p;
	::SetWindowPos((HWND)p.winId(), HWND_TOPMOST, 0, 0, p.width(), p.height(), SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	int ret = a.exec();

	::CloseHandle(m_hMutex);
	m_hMutex = NULL;
	return ret;
}
