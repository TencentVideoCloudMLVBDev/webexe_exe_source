#pragma once
#include "commonType.h"

class DataReport
{
private:
	DataReport();

public:
	virtual ~DataReport();

	/**
	* \brief：获取DataReport单例，通过单例调用DataReport的接口
	*/
	static DataReport& instance();

	void setExeLaunch(uint64_t ts);
	void setCGILogin(uint64_t ts);
	void setIMLogin(uint64_t ts);
	void setEnterDemo(uint64_t ts);
	void setCGIPushURL(uint64_t ts);
	void setConnectSucc(uint64_t ts);
	void setPushBegin(uint64_t ts);
	void setCGICreateRoom(uint64_t ts);
	void setCGIAddPusher(uint64_t ts);
	void setRecord(bool record);
	void setProxy(bool proxy);
	void setRoomType(std::string str_roomType);
	void setRecordScreen(uint32_t recordScreenType);
	void setLocalHttp(uint64_t ts);
	void setRoomInfo(uint32_t int32_appid, std::string str_roomid, bool bool_room_creator, std::string str_userid, std::string str_nickname);
	std::string getInitReport();
	uint64_t txf_gettickcount();
	uint64_t txf_gettickspan(uint64_t lastTick);

	void setFetchCosSigCode(uint32_t code);
	void setUploadUrl(std::string str_uploadUrl);
	void setUploadtoCosCode(uint32_t code);
	void setPreviewUrl(std::string str_previewUrl);
	void setPageCount(uint32_t count);
	void setFileSize(uint32_t size);
	void setClickUpload(uint64_t ts);
	void setPreview(uint64_t ts);
	std::string getWhiteboardReport();

private:
	InitDataReport m_initReport;
	WhiteboardReport m_whiteBoardReport;
};
