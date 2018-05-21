#include "DataReport.h"
#include <time.h>
#include <windows.h>

DataReport::DataReport()
{
}

DataReport::~DataReport()
{
}

DataReport * DataReport::instance()
{
	static DataReport *dataReport = new DataReport;
	return dataReport;
}

void DataReport::setRcvProtol(long long ts)
{
	m_createReport.int64_ts_rcv_protol = ts;
}

void DataReport::setLogin(long long ts)
{
	m_createReport.int64_ts_login = ts;
}

void DataReport::setIMLogin(long long tc)
{
	m_createReport.int64_tc_im_login = tc;
}

void DataReport::setCreate(long long ts)
{
	m_createReport.int64_ts_create = ts;
}

void DataReport::setCGIPushURL(long long tc)
{
	m_createReport.int64_tc_cgi_get_pushurl = tc;
}

void DataReport::setConnectSucc(long long ts)
{
	m_createReport.int64_ts_connect_succ = ts;
}

void DataReport::setPushBegin(long long ts)
{
	m_createReport.int64_ts_push_begin = ts;
}

void DataReport::setCGICreateRoom(long long tc)
{
	m_createReport.int64_tc_cgi_create_room = tc;
}

void DataReport::setCreateRoom(long long tc)
{
	m_createReport.int64_tc_create_room = tc;
}

void DataReport::generateCreateReport(uint32_t int32_appid, std::string str_roomid, std::string str_room_creator, std::string str_nickname, std::string str_push_info, uint32_t int32_is_roomservice)
{
	m_createReport.int64_tc_create_room = txf_gettickspan(m_createReport.int64_ts_rcv_protol);
	m_createReport.str_roomid = str_roomid;
	m_createReport.str_room_creator = str_room_creator;
	m_createReport.str_nickname = str_nickname;
	m_createReport.str_push_info = str_push_info;
	m_createReport.int32_is_roomservice = int32_is_roomservice;
}

CreateDataReport DataReport::getCreateReport()
{
	return m_createReport;
}

int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}

uint64_t DataReport::txf_gettickspan(uint64_t lastTick) {
	struct timeval currentWin;
	gettimeofday(&currentWin, NULL);
	unsigned long long tickGet = (unsigned long long)currentWin.tv_sec * 1000 + currentWin.tv_usec / 1000;

	if (tickGet >= lastTick)
	{
		return (tickGet - lastTick);
	}
	else
	{
		return 0;
	}
}

uint64_t DataReport::txf_gettickcount() {
	static unsigned long long s_TickDelta = 0;

	unsigned long long tickGet = 0;

	struct timeval currentWin;
	gettimeofday(&currentWin, NULL);
	tickGet = (unsigned long long)currentWin.tv_sec * 1000 + currentWin.tv_usec / 1000;

	if (s_TickDelta == 0) {
		struct timeval current;
		gettimeofday(&current, NULL);
		s_TickDelta = ((unsigned long long)current.tv_sec * 1000 + current.tv_usec / 1000) - tickGet;
	}

	tickGet += s_TickDelta;

	return tickGet;
}