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
	static DataReport* instance();

	void setRcvProtol(long long ts);
	void setLogin(long long ts);
	void setIMLogin(long long tc);
	void setCreate(long long ts);
	void setCGIPushURL(long long tc);
	void setConnectSucc(long long ts);
	void setPushBegin(long long ts);
	void setCGICreateRoom(long long tc);
	void setCreateRoom(long long tc);
	void generateCreateReport(uint32_t int32_appid, std::string str_roomid, std::string str_room_creator, std::string str_nickname,std::string str_push_info, uint32_t int32_is_roomservice);
	CreateDataReport getCreateReport();

	uint64_t txf_gettickcount();
	uint64_t txf_gettickspan(uint64_t lastTick);
private:
	CreateDataReport m_createReport;
};
