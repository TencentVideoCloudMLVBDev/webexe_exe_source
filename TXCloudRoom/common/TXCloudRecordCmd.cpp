#include "TXCloudRecordCmd.h"
#include <Psapi.h>
#include <Shellapi.h>
#include <assert.h>
#include <shellapi.h>
#include <TlHelp32.h>

#pragma comment (lib,"Psapi.lib")

#define WINDOW_CLASS_NAME TEXT("TXCloudRecord")
#define WINDOW_CAPTION_NAME TEXT("TXCloudRecordCaption")
#define RecordExe "TXCloudRecord.exe"

static unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

static unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

static std::string URLEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}

static std::string URLDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

TXCloudRecordCmd::TXCloudRecordCmd()
    : m_recordHwnd(nullptr)
    , m_isExits(false)
{
}

TXCloudRecordCmd::~TXCloudRecordCmd()
{
	stop();
}

TXCloudRecordCmd& TXCloudRecordCmd::instance()
{
	static TXCloudRecordCmd uniqueInstance;
	return uniqueInstance;
}

bool TXCloudRecordCmd::runAndRecord(RecordData recordData)
{
	BOOL ret = FALSE;
	if (recordData.recordType == RecordScreenNone || (recordData.recordUrl[0] == '\0' && recordData.recordPath[0] == '\0')
        || (recordData.recordExe[0] == '\0' && recordData.winID == -1))
	{
		return ret;
	}

	do
	{
		Json::Value root;
		root["recordUrl"] = recordData.recordUrl;
		root["recordPath"] = recordData.recordPath;
		root["sliceTime"] = recordData.sliceTime;
		root["recordExe"] = recordData.recordExe; //需要录制的exe名称。启动record之前就需要运行
		//root["winID"] = (int)GetDesktopWindow();  //需要录制的窗口句柄。winID和recordExe必须传一个，都传会录制recordExe内容
		if (recordData.recordExe[0] == '\0' && recordData.winID != -1)
		{
			root["winID"] = recordData.winID;
		}

		root["recordType"] = recordData.recordType;
        root["parentPid"] = static_cast<uint64_t>(::GetCurrentProcessId()); // 传入父进程pid，record进程会监听父进程是否存活

		Json::FastWriter writer;
		std::string jsonUTF8 = writer.write(root);
		std::string encodeJson = URLEncode(jsonUTF8);

		std::string cmd = "\"txcloudrecord://liteav/params?json=";
		cmd.append(encodeJson);
		cmd.append("\\");

		char szFilePath[MAX_PATH + 1] = { 0 };
		GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
		(strrchr(szFilePath, '\\'))[0] = 0;
		std::string path = szFilePath;
		path.append("\\");
		path.append(RecordExe);

		
		SHELLEXECUTEINFOA sei = { 0 };
		sei.cbSize = sizeof(SHELLEXECUTEINFOA);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.hwnd = NULL;
		sei.lpVerb = "open";
		sei.lpFile = path.c_str();
		sei.lpParameters = cmd.c_str();
		sei.lpDirectory = NULL;
		sei.nShow = SW_HIDE;
		sei.hInstApp = NULL;
		sei.lpIDList = NULL;
		sei.lpClass = NULL;
		sei.hkeyClass = NULL;
		sei.dwHotKey = NULL;
		sei.hIcon = NULL;
		sei.hProcess = NULL;

		ret = ::ShellExecuteExA(&sei);
		if (FALSE == ret)
		{
			LERROR(L"ShellExecuteExW failed: %lu", ::GetLastError());
			return false;
		}

        m_isExits = true;
	} while (0);
	return ret;
}

void TXCloudRecordCmd::update(RecordData recordData)
{
	LINFO(L"txcloudrecord update");

	if (!m_recordHwnd)
	{
		m_recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
	}

	if (m_recordHwnd)
	{
		COPYDATASTRUCT copy_data = { ScreenRecordUpdate, sizeof(recordData), &recordData};

        DWORD_PTR dwResult = 0;
		::SendMessageTimeout(m_recordHwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(m_recordHwnd), reinterpret_cast<LPARAM>(&copy_data), SMTO_BLOCK, 3000, &dwResult);
	}
}

void TXCloudRecordCmd::exit()
{
	LINFO(L"txcloudrecord exit");

	if (!m_recordHwnd)
	{
		m_recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
	}

	if (m_recordHwnd)
	{
		COPYDATASTRUCT copy_data = { ScreenRecordExit, 0, NULL };

        DWORD_PTR dwResult = 0;
        ::SendMessageTimeout(m_recordHwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(m_recordHwnd), reinterpret_cast<LPARAM>(&copy_data), SMTO_BLOCK, 3000, &dwResult);
	}

    m_isExits = false;
	m_recordHwnd = nullptr;
}

void TXCloudRecordCmd::start()
{
	LINFO(L"txcloudrecord start");

	if (!m_recordHwnd)
	{
		m_recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
	}

	if (m_recordHwnd)
	{
		COPYDATASTRUCT copy_data = { ScreenRecordStart, 0, NULL };

        DWORD_PTR dwResult = 0;
        ::SendMessageTimeout(m_recordHwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(m_recordHwnd), reinterpret_cast<LPARAM>(&copy_data), SMTO_BLOCK, 3000, &dwResult);
	}
}

void TXCloudRecordCmd::stop()
{
	LINFO(L"txcloudrecord stop");

	if (!m_recordHwnd)
	{
		m_recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
	}

	if (m_recordHwnd)
	{
		COPYDATASTRUCT copy_data = { ScreenRecordStop, 0, NULL };

        DWORD_PTR dwResult = 0;
        ::SendMessageTimeout(m_recordHwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(m_recordHwnd), reinterpret_cast<LPARAM>(&copy_data), SMTO_BLOCK, 3000, &dwResult);
	}
}

bool TXCloudRecordCmd::isExist()
{
    if (false == m_isExits)
    {
        return m_isExits;
    }
    else
    {
        m_recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
    }
}

void TXCloudRecordCmd::cleanProcess()
{
    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(pe32);

    HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    BOOL bResult = ::Process32First(hProcessSnap, &pe32);
    while (bResult)
    {
        if (0 == ::wcscmp(pe32.szExeFile, L"TXCloudRecord.exe"))
        {
            HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
            ::TerminateProcess(hProcess, -1);
            ::CloseHandle(hProcess);
        }

        bResult = Process32Next(hProcessSnap, &pe32);
    }

    ::CloseHandle(hProcessSnap);
}
