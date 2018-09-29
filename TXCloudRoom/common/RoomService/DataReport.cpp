#include "DataReport.h"
#include <time.h>
#include <windows.h>

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
	m_enterReport.int64_ts_exe_launch = ts;
}

void DataReport::setCGILogin(uint64_t ts)
{
	m_enterReport.int64_ts_cgi_login = ts;
}

void DataReport::setIMLogin(uint64_t tc)
{
	m_enterReport.int64_ts_im_login = tc;
}

void DataReport::setEnterDemo(uint64_t ts)
{
	m_enterReport.int64_ts_enter_demo = ts;
}

void DataReport::setCGIPushURL(uint64_t tc)
{
	m_enterReport.int64_ts_cgi_get_pushurl = tc;
}

void DataReport::setConnectSucc(uint64_t ts)
{
	m_enterReport.int64_ts_connect_succ = ts;
}

void DataReport::setPushBegin(uint64_t ts)
{
	m_enterReport.int64_ts_push_begin = ts;
}

void DataReport::setCGICreateRoom(uint64_t tc)
{
	m_enterReport.int64_ts_cgi_create_room = tc;
}

void DataReport::setCGIAddPusher(uint64_t ts)
{
	m_enterReport.int64_ts_cgi_add_pusher = ts;
}

void DataReport::setRecord(bool record)
{
	m_enterReport.bool_record = record;
}

void DataReport::setProxy(bool proxy)
{
	m_enterReport.bool_proxy = proxy;
	m_whiteBoardUploadReport.bool_proxy = proxy;
}

void DataReport::setRoomType(std::string str_roomType)
{
	m_commonReport.str_room_type = str_roomType;
}

void DataReport::setRecordScreen(uint32_t recordScreenType)
{
	m_enterReport.int32_record_screen = recordScreenType;
}

void DataReport::setLocalHttp(uint64_t ts)
{
	m_enterReport.int64_ts_local_http = ts;
}

void DataReport::setUserInfo(uint32_t int32_appid, std::string str_userid, std::string str_nickname, bool bool_room_creator)
{
	m_commonReport.int32_sdkappid = int32_appid;
	m_commonReport.str_userid = str_userid;
	m_commonReport.str_nickname = str_nickname;
	m_commonReport.bool_room_creator = bool_room_creator;
}

void DataReport::setRoomInfo(std::string str_roomid)
{
	m_commonReport.str_roomid = str_roomid;
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
	m_whiteBoardUploadReport.int32_fetchcossig_code = code;
}

void DataReport::setUploadUrl(std::string str_uploadUrl)
{
	m_whiteBoardUploadReport.str_uploadurl = str_uploadUrl;
}

void DataReport::setUploadtoCosCode(uint32_t code)
{
	m_whiteBoardUploadReport.int32_uploadtocos_code = code;
}

void DataReport::setPreviewUrl(std::string str_previewUrl)
{
	m_whiteBoardUploadReport.str_previewurl = str_previewUrl;
}

void DataReport::setPageCount(int32_t count)
{
	m_whiteBoardUploadReport.int32_pagecount = count;
}

void DataReport::setFileSize(uint32_t size)
{
	m_whiteBoardUploadReport.int32_filesize = size;
}

void DataReport::setClickUpload(uint64_t ts)
{
	m_whiteBoardUploadReport.int64_ts_click_upload = ts;
}

void DataReport::setPreview(uint64_t ts)
{
	m_whiteBoardUploadReport.int64_ts_preview = ts;
}

void DataReport::setStreamID(std::string str_stream_id)
{
	m_streamReport.str_stream_id = str_stream_id;
}

void DataReport::setStreamAction(std::string str_stream_action)
{
	m_streamReport.str_action = str_stream_action;
}

void DataReport::setResult(DataReportType type, std::string result, std::string reason)
{
	switch (type)
	{
	case DataReportEnter:
	{
		if (!m_enterReport.str_result.empty())
		{
			break;;
		}
		m_enterReport.str_result = result;
		m_enterReport.str_reason = reason;
	}
		break;
	case DataReportLeave:
	{
		m_leaveReport.str_result = result;
		m_leaveReport.str_reason = reason;
	}
		break;
	case DataReportError:
	{
		m_errorReport.str_result = result;
		m_errorReport.str_reason = reason;
	}
		break;
	case DataReportWBupload:
	{
		m_whiteBoardUploadReport.str_result = result;
		m_whiteBoardUploadReport.str_reason = reason;
	}
		break;
	case DataReportWBLast:
	{
		m_whiteBoardLastReport.str_result = result;
		m_whiteBoardLastReport.str_reason = reason;
	}
		break;
	case DataReportWBNext:
	{
		m_whiteBoardNextReport.str_result = result;
		m_whiteBoardNextReport.str_reason = reason;
	}
		break;
	case DataReportStream:
	{
		m_streamReport.str_result = result;
		m_streamReport.str_reason = reason;
	}
	break;
	default:
		break;
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

void DataReport::getCommonReport(Json::Value & root)
{
	root["type"] = m_commonReport.type;
	root["bussiness"] = m_commonReport.bussiness;
	root["platform"] = m_commonReport.platform;
	root["str_app_name"] = m_commonReport.str_app_name;
	root["str_token"] = m_commonReport.str_token;
	root["int32_sdkapp_id"] = m_commonReport.int32_sdkappid;

	root["str_room_type"] = m_commonReport.str_room_type;
	root["str_roomid"] = m_commonReport.str_roomid;
	root["bool_room_creator"] = m_commonReport.bool_room_creator;
	root["str_userid"] = m_commonReport.str_userid;
	root["str_nickname"] = m_commonReport.str_nickname;

	root["str_device_type"] = m_commonReport.str_device_type;
	root["str_app_version"] = m_commonReport.str_app_version;
	root["str_sdk_version"] = m_commonReport.str_sdk_version;
}

std::string DataReport::getEnterReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["int64_ts_exe_launch"] = m_enterReport.int64_ts_exe_launch;
	root["int64_ts_enter_demo"] = m_enterReport.int64_ts_enter_demo;
	root["int64_ts_local_http"] = m_enterReport.int64_ts_local_http;
	root["int64_ts_cgi_login"] = m_enterReport.int64_ts_cgi_login;
	root["int64_ts_im_login"] = m_enterReport.int64_ts_im_login;
	root["int64_ts_cgi_get_pushurl"] = m_enterReport.int64_ts_cgi_get_pushurl;
	root["int64_ts_connect_succ"] = m_enterReport.int64_ts_connect_succ;
	root["int64_ts_push_begin"] = m_enterReport.int64_ts_push_begin;
	root["int64_ts_cgi_create_room"] = m_enterReport.int64_ts_cgi_create_room;
	root["int64_ts_cgi_add_pusher"] = m_enterReport.int64_ts_cgi_add_pusher;

	root["bool_proxy"] = m_enterReport.bool_proxy;
	root["bool_record"] = m_enterReport.bool_record;
	root["int32_record_screen"] = m_enterReport.int32_record_screen;

	root["str_action"] = m_enterReport.str_action;
	root["str_result"] = m_enterReport.str_result;
	root["str_reason"] = m_enterReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getLeaveReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["str_action"] = m_leaveReport.str_action;
	root["str_result"] = m_leaveReport.str_result;
	root["str_reason"] = m_leaveReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getErrorReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["str_action"] = m_errorReport.str_action;
	root["str_result"] = m_errorReport.str_result;
	root["str_reason"] = m_errorReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getWhiteboardUploadReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["int32_fetchcossig_code"] = m_whiteBoardUploadReport.int32_fetchcossig_code;
	root["str_uploadurl"] = m_whiteBoardUploadReport.str_uploadurl;
	root["int32_uploadtocos_code"] = m_whiteBoardUploadReport.int32_uploadtocos_code;
	root["str_previewurl"] = m_whiteBoardUploadReport.str_previewurl;
	root["int32_pagecount"] = m_whiteBoardUploadReport.int32_pagecount;
	root["int32_filesize"] = m_whiteBoardUploadReport.int32_filesize;
	root["int64_ts_click_upload"] = m_whiteBoardUploadReport.int64_ts_click_upload;
	root["int64_ts_preview"] = m_whiteBoardUploadReport.int64_ts_preview;

	root["bool_proxy"] = m_whiteBoardUploadReport.bool_proxy;

	root["str_action"] = m_whiteBoardUploadReport.str_action;
	root["str_result"] = m_whiteBoardUploadReport.str_result;
	root["str_reason"] = m_whiteBoardUploadReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getWhiteboardLastReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["str_action"] = m_whiteBoardLastReport.str_action;
	root["str_result"] = m_whiteBoardLastReport.str_result;
	root["str_reason"] = m_whiteBoardLastReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getWhiteboardNextReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["str_action"] = m_whiteBoardNextReport.str_action;
	root["str_result"] = m_whiteBoardNextReport.str_result;
	root["str_reason"] = m_whiteBoardNextReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;
}

std::string DataReport::getStreamReport()
{
	std::string jsonUTF8;
	Json::Value root;
	getCommonReport(root);

	root["str_stream_id"] = m_streamReport.str_stream_id;
	root["str_action"] = m_streamReport.str_action;
	root["str_result"] = m_streamReport.str_result;
	root["str_reason"] = m_streamReport.str_reason;

	Json::FastWriter writer;
	jsonUTF8 = writer.write(root);
	return jsonUTF8;

}
