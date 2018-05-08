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
