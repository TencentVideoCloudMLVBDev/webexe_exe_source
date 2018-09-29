#pragma once
#include "commonType.h"
#include "json.h"

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

	uint64_t txf_gettickcount();
	uint64_t txf_gettickspan(uint64_t lastTick);

	//enter
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
	void setRecordScreen(uint32_t recordScreenType);
	void setLocalHttp(uint64_t ts);
	//common
	void setRoomType(std::string str_roomType);
	void setUserInfo(uint32_t int32_appid, std::string str_userid, std::string str_nickname, bool bool_room_creator);
	void setRoomInfo(std::string str_roomid);
	
	//whiteboard
	void setFetchCosSigCode(uint32_t code);
	void setUploadUrl(std::string str_uploadUrl);
	void setUploadtoCosCode(uint32_t code);
	void setPreviewUrl(std::string str_previewUrl);
	void setPageCount(int32_t count);
	void setFileSize(uint32_t size);
	void setClickUpload(uint64_t ts);
	void setPreview(uint64_t ts);

	//stream
	void setStreamID(std::string str_stream_id);
	void setStreamAction(std::string str_stream_action);

	//result
	void setResult(DataReportType type, std::string result, std::string reason = "");

	//getreport
	void getCommonReport(Json::Value & root);
	std::string getEnterReport();
	std::string getLeaveReport();
	std::string getErrorReport();
	std::string getWhiteboardUploadReport();
	std::string getWhiteboardLastReport();
	std::string getWhiteboardNextReport();
	std::string getStreamReport();

private:
	EnterDataReport m_enterReport;
	LeaveDataReport m_leaveReport;
	ErrorDataReport m_errorReport;
	WhiteboardUploadReport m_whiteBoardUploadReport;
	WhiteboardLastReport m_whiteBoardLastReport;
	WhiteboardNextReport m_whiteBoardNextReport;
	StreamDataReport m_streamReport;
	CommonDataReport m_commonReport;
};
