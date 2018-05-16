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

struct MemberItem
{
    MemberRole role;
	std::string userID;
	std::string userName;
	std::string userAvatar;
    std::string status;
};

struct CreateDataReport
{
	uint32_t int32_appid;
	std::string str_roomid;
	std::string str_room_creator;
	uint64_t int64_ts_rcv_protol;
	uint64_t int64_ts_login;
	uint64_t int64_tc_im_login;
	uint64_t int64_ts_create;
	uint64_t int64_tc_cgi_get_pushurl;
	uint64_t int64_ts_connect_succ;
	uint64_t int64_ts_push_begin;
	uint64_t int64_tc_cgi_create_room;
	uint64_t int64_tc_create_room;

	std::string str_appversion;
	std::string str_sdkversion;
	std::string str_nickname;
	std::string str_device_type;
	std::string str_push_info;
	uint32_t int32_is_roomservice;

	CreateDataReport()
	{
		this->int32_appid = -1;
		this->int64_ts_rcv_protol = -1;
		this->int64_ts_login = -1;
		this->int64_tc_im_login = -1;
		this->int64_ts_create = -1;
		this->int64_tc_cgi_get_pushurl = -1;
		this->int64_ts_connect_succ = -1;
		this->int64_ts_push_begin = -1;
		this->int64_tc_cgi_create_room = -1;
		this->int64_tc_create_room = -1;
		this->int32_is_roomservice = -1;
	}
};
