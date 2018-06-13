#pragma once
#include <string>

struct MenuInfo
{
	bool mainDis;
	bool camera;
	bool mic;
	bool fullScreen;
	bool linkMic;
	MenuInfo()
	{
		this->mainDis = false;
		this->camera = true;
		this->mic = true;
		this->fullScreen = false;
		this->linkMic = false;
	}
};

struct BoardAuthData
{
	int sdkAppID;
	std::string userID;
	std::string token;
};

enum MemberRole
{
    MasterPusherRole = 0,
    SlavePusherRole = 1,
    AudienceRole = 2,
};

enum ScreenRecordType
{
	RecordScreenNone = 0,
	RecordScreenToServer = 1,
	RecordScreenToClient = 2,
	RecordScreenToBoth = 3,
};

enum ScreenRecordCmd
{
	ScreenRecordStart = 1001,
	ScreenRecordStop = 1002,
	ScreenRecordExit = 1003,
};

struct MemberItem
{
    MemberRole role;
	std::string userID;
	std::string userName;
	std::string userAvatar;
    std::string status;
};

enum DataReportType
{
	DataReportEnter = 0,
	DataReportLeave = 1,
	DataReportError = 2,
	DataReportWBupload = 3,
	DataReportWBLast = 4,
	DataReportWBNext = 5,
};

struct CommonDataReport
{
	std::string type;
	std::string str_app_name;
	std::string str_token;
	uint32_t int32_sdkapp_id;

	std::string str_room_type;
	std::string str_roomid;
	bool bool_room_creator;
	std::string str_userid;
	std::string str_nickname;

	std::string str_device_type;
	std::string str_app_version;
	std::string str_sdk_version;

	CommonDataReport()
	{
		this->int32_sdkapp_id = 0;
		this->type = "webexe";
		this->str_app_name = "TXCloudRoom";
	}
};

struct EnterDataReport
{
	uint64_t int64_ts_exe_launch;
	uint64_t int64_ts_enter_demo;
	uint64_t int64_ts_local_http;
	uint64_t int64_ts_cgi_login;
	uint64_t int64_ts_im_login;
	uint64_t int64_ts_cgi_get_pushurl;
	uint64_t int64_ts_connect_succ;
	uint64_t int64_ts_push_begin;
	uint64_t int64_ts_cgi_create_room;
	uint64_t int64_ts_cgi_add_pusher;

	bool bool_proxy;
	bool bool_record;
	uint32_t int32_record_screen;

	std::string str_action;
	std::string str_result;
	std::string str_reason;

	EnterDataReport()
	{
		this->int64_ts_exe_launch = 0;
		this->int64_ts_enter_demo = 0;
		this->int64_ts_local_http = 0;
		this->int64_ts_cgi_login = 0;
		this->int64_ts_im_login = 0;
		this->int64_ts_cgi_get_pushurl = 0;
		this->int64_ts_connect_succ = 0;
		this->int64_ts_push_begin = 0;
		this->int64_ts_cgi_create_room = 0;
		this->int64_ts_cgi_add_pusher = 0;

		this->bool_proxy = false;
		this->bool_record = false;
		this->int32_record_screen = 0;

		this->str_action = "enter";
	}
};

struct LeaveDataReport
{
	std::string str_action;
	std::string str_result;
	std::string str_reason;

	LeaveDataReport()
	{
		this->str_action = "leave";
	}
};

struct ErrorDataReport
{
	std::string str_action;
	std::string str_result;
	std::string str_reason;

	ErrorDataReport()
	{
		this->str_action = "error";
	}
};

struct WhiteboardUploadReport
{
	uint32_t int32_fetchcossig_code;
	std::string str_uploadurl;
	uint32_t int32_uploadtocos_code;
	std::string str_previewurl;
	uint32_t int32_pagecount;
	uint32_t int32_filesize;
	uint64_t int64_ts_click_upload;
	uint64_t int64_ts_preview;
	
	bool bool_proxy;

	std::string str_action;
	std::string str_result;
	std::string str_reason;

	WhiteboardUploadReport()
	{
		this->int32_fetchcossig_code = 0;
		this->int32_uploadtocos_code = 0;
		this->int32_pagecount = 0;
		this->int32_filesize = 0;
		this->int64_ts_click_upload = 0;
		this->int64_ts_preview = 0;

		this->bool_proxy = false;

		this->str_action = "whiteboard_upload";
	}
};

struct WhiteboardLastReport
{
	std::string str_action;
	std::string str_result;
	std::string str_reason;

	WhiteboardLastReport()
	{
		this->str_action = "whiteboard_last";
	}
};

struct WhiteboardNextReport
{
	std::string str_action;
	std::string str_result;
	std::string str_reason;

	WhiteboardNextReport()
	{
		this->str_action = "whiteboard_next";
	}
};