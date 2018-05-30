#include "DataReport.h"
#include <time.h>
#include <windows.h>
#include "json.h"

DataReport::DataReport()
{
}

DataReport::~DataReport()
{
}

DataReport & DataReport::instance()
{
	static DataReport dataReport;
	return dataReport;
}

void DataReport::setExeLaunch(uint64_t ts)
{
	m_initReport.int64_ts_exe_launch = ts;
}

void DataReport::setCGILogin(uint64_t ts)
{
	m_initReport.int64_ts_cgi_login = ts;
}

void DataReport::setIMLogin(uint64_t tc)
{
	m_initReport.int64_ts_im_login = tc;
}

void DataReport::setEnterDemo(uint64_t ts)
{
	m_initReport.int64_ts_enter_demo = ts;
}

void DataReport::setCGIPushURL(uint64_t tc)
{
	m_initReport.int64_ts_cgi_get_pushurl = tc;
}

void DataReport::setConnectSucc(uint64_t ts)
{
	m_initReport.int64_ts_connect_succ = ts;
}

void DataReport::setPushBegin(uint64_t ts)
{
	m_initReport.int64_ts_push_begin = ts;
}

void DataReport::setCGICreateRoom(uint64_t tc)
{
	m_initReport.int64_ts_cgi_create_room = tc;
}

void DataReport::setCGIAddPusher(uint64_t ts)
{
	m_initReport.int64_ts_cgi_add_pusher = ts;
}

void DataReport::setRecord(bool record)
{
	m_initReport.bool_record = record;
}

void DataReport::setProxy(bool proxy)
{
	m_initReport.bool_proxy = proxy;
	m_whiteBoardReport.bool_proxy = proxy;
}

void DataReport::setRoomType(std::string str_roomType)
{
	m_initReport.str_room_type = str_roomType;
	m_whiteBoardReport.str_room_type = str_roomType;
}

void DataReport::setRecordScreen(uint32_t recordScreenType)
{
	m_initReport.int32_record_screen = recordScreenType;
}

void DataReport::setLocalHttp(uint64_t ts)
{
	m_initReport.int64_ts_local_http = ts;
}

void DataReport::setRoomInfo(uint32_t int32_appid, std::string str_roomid, bool bool_room_creator, std::string str_userid, std::string str_nickname)
{
	m_initReport.str_roomid = str_roomid;
	m_initReport.bool_room_creator = bool_room_creator;
	m_initReport.str_userid = str_userid;
	m_initReport.str_nickname = str_nickname;

	m_whiteBoardReport.str_roomid = str_roomid;
	m_whiteBoardReport.bool_room_creator = bool_room_creator;
	m_whiteBoardReport.str_userid = str_userid;
	m_whiteBoardReport.str_nickname = str_nickname;
}

std::string DataReport::getInitReport()
{
	std::string jsonUTF8;
	Json::Value root;
	root["type"] = "webexe";
	root["str_app_name"] = "TXCloudRoom.exe";
	root["str_token"] = m_initReport.str_token;
	root["int32_app_id"] = m_initReport.int32_appid;

	root["int64_ts_exe_launch"] = m_initReport.int64_ts_exe_launch;
	root["int64_ts_enter_demo"] = m_initReport.int64_ts_enter_demo;
	root["int64_ts_local_http"] = m_initReport.int64_ts_local_http;
	root["int64_ts_cgi_login"] = m_initReport.int64_ts_cgi_login;
	root["int64_ts_im_login"] = m_initReport.int64_ts_im_login;
	root["int64_ts_cgi_get_pushurl"] = m_initReport.int64_ts_cgi_get_pushurl;
	root["int64_ts_connect_succ"] = m_initReport.int64_ts_connect_succ;
	root["int64_ts_push_begin"] = m_initReport.int64_ts_push_begin;
	root["int64_ts_cgi_create_room"] = m_initReport.int64_ts_cgi_create_room;
	root["int64_ts_cgi_add_pusher"] = m_initReport.int64_ts_cgi_add_pusher;

	root["bool_proxy"] = m_initReport.bool_proxy;
	root["bool_record"] = m_initReport.bool_record;
	root["int32_record_screen"] = m_initReport.int32_record_screen;

	root["str_room_type"] = m_initReport.str_room_type;
	root["str_roomid"] = m_initReport.str_roomid;
	root["bool_room_creator"] = m_initReport.bool_room_creator;
	root["str_userid"] = m_initReport.str_userid;
	root["str_nickname"] = m_initReport.str_nickname;

	root["str_device_type"] = m_initReport.str_device_type;
	root["str_app_version"] = m_initReport.str_app_version;
	root["str_sdk_version"] = m_initReport.str_sdk_version;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
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

void DataReport::setFetchCosSigCode(uint32_t code)
{
	m_whiteBoardReport.int32_fetchcossig_code = code;
}

void DataReport::setUploadUrl(std::string str_uploadUrl)
{
	m_whiteBoardReport.str_uploadurl = str_uploadUrl;
}

void DataReport::setUploadtoCosCode(uint32_t code)
{
	m_whiteBoardReport.int32_uploadtocos_code = code;
}

void DataReport::setPreviewUrl(std::string str_previewUrl)
{
	m_whiteBoardReport.str_previewurl = str_previewUrl;
}

void DataReport::setPageCount(uint32_t count)
{
	m_whiteBoardReport.int32_pagecount = count;
}

void DataReport::setFileSize(uint32_t size)
{
	m_whiteBoardReport.int32_filesize = size;
}

void DataReport::setClickUpload(uint64_t ts)
{
	m_whiteBoardReport.int64_ts_click_upload = ts;
}

void DataReport::setPreview(uint64_t ts)
{
	m_whiteBoardReport.int64_ts_preview = ts;
}

std::string DataReport::getWhiteboardReport()
{
	std::string jsonUTF8;
	Json::Value root;
	root["type"] = "webexe";
	root["str_app_name"] = "TXCloudRoom.exe";
	root["str_token"] = m_whiteBoardReport.str_token;
	root["int32_app_id"] = m_whiteBoardReport.int32_appid;

	root["int32_fetchcossig_code"] = m_whiteBoardReport.int32_fetchcossig_code;
	root["str_uploadurl"] = m_whiteBoardReport.str_uploadurl;
	root["int32_uploadtocos_code"] = m_whiteBoardReport.int32_uploadtocos_code;
	root["str_previewurl"] = m_whiteBoardReport.str_previewurl;
	root["int32_pagecount"] = m_whiteBoardReport.int32_pagecount;
	root["int32_filesize"] = m_whiteBoardReport.int32_filesize;
	root["int64_ts_upload"] = m_whiteBoardReport.int64_ts_click_upload;
	root["int64_ts_preview"] = m_whiteBoardReport.int64_ts_preview;

	root["bool_proxy"] = m_whiteBoardReport.bool_proxy;

	root["str_room_type"] = m_whiteBoardReport.str_room_type;
	root["str_roomid"] = m_whiteBoardReport.str_roomid;
	root["bool_room_creator"] = m_whiteBoardReport.bool_room_creator;
	root["str_userid"] = m_whiteBoardReport.str_userid;
	root["str_nickname"] = m_whiteBoardReport.str_nickname;

	root["str_device_type"] = m_whiteBoardReport.str_device_type;
	root["str_app_version"] = m_whiteBoardReport.str_app_version;
	root["str_sdk_version"] = m_whiteBoardReport.str_sdk_version;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
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