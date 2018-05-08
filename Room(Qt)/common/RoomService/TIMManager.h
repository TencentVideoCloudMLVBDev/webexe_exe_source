#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

#include "tim_c.h"
#include "tim_group_c.h"

enum IMGroupOptType
{
	kgCreateGroup,
	kgJoinGroup,
	kgQuitGroup,
	kgDeleteGroup,
};

struct IMAccountInfo
{
	std::string  userID;
	std::string  userSig;
	int  sdkAppID;
	std::string  accType;
};

class IMRecvMsgCallback
{
public:
	virtual ~IMRecvMsgCallback() = default;
	virtual void onRecvC2CTextMsg(const char * userID, const char * msg) = 0;
	virtual void onRecvGroupTextMsg(const char * groupID, const char * userID, const char * msg, const char * msgHead = nullptr) = 0;

	virtual void onRecvC2CCustomMsg(const char * userID, const char * msg) = 0;
	virtual void onRecvGroupCustomMsg(const char * groupID, const char * userID, const char * msg) = 0;

	virtual void onRecvGroupSystemMsg(const char * groupID, const char * msg) = 0;
};

class IMGroupChangeCallback
{
public:
	virtual ~IMGroupChangeCallback() = default;
	virtual void onGroupChangeMessage(IMGroupOptType opType, const char* group_id, const char * user_id) = 0;
};

class IMGroupOptCallBack
{
public:
    virtual ~IMGroupOptCallBack() = default;
	virtual void onGroupOptSuccess(IMGroupOptType opType, const char* group_id) = 0;
	virtual void onGroupOptError(IMGroupOptType opType, const char* group_id, int code, const char* desc) = 0;
};

class IMRecvWBDataCallback
{
public:
	virtual ~IMRecvWBDataCallback() = default;
	virtual void onRecvWhiteBoardData(const char *data, uint32_t length) = 0;
};

class IMSendMsgCallBack
{
public:
    virtual ~IMSendMsgCallBack() = default;
	virtual void onSendMsg(int err, const char * errMsg) = 0;
};

class IMSendWBDataCallBack
{
public:
    virtual ~IMSendWBDataCallBack() = default;
	virtual void onSendWBData(int err, const char * errMsg) = 0;
};

class TIMManager
{
public:
	TIMManager();
	virtual ~TIMManager();

	static TIMManager * instance();

    void init(const std::string& ip, unsigned short port);

	void login(const IMAccountInfo & IMInfo, TIMCommCB *callback);
	void logout();

	void setMsgHead(const char * msgHead);

	void opGroup(IMGroupOptType opType, const char* group_id, IMGroupOptCallBack* sink);

	void setRecvMsgCallBack(IMRecvMsgCallback * sink);
	void setRecvWBDataCallBack(IMRecvWBDataCallback * sink);
	void setSendMsgCallBack(IMSendMsgCallBack * sink);
	void setSendWBDataCallBack(IMSendWBDataCallBack * sink);
	void setGroupChangeCallBack(IMGroupChangeCallback * sink);

	void sendC2CTextMsg(const char * dstUser, const char * msg);
	void sendGroupTextMsg(const char* groupId, const char * msg);

	void sendC2CCustomMsg(const char * dstUser, const char * msg);
	void sendGroupCustomMsg(const char* groupId, const char * msg);

	void sendWhiteBoardData(const char* groupId, const char* data, uint32_t length);
private:
	int sendMsg(TIMConversationType type, const char * peer, TIMConversationHandle msg);
	void onRecvWBData(const std::string& data);
	//static void onGroupTips(TIMMsgGroupTipsElemHandle group_tips_elems, void* data);
    static void onTIMNewMessage(TIMMessageHandle* handles, uint32_t msg_num, void* data);
    static void onTIMKickOffline(void* data);
	static void OnGetWBFileDataSuccess(const char* buf, uint32_t len, void* data);
	static void OnGetWBFileDataError(int code, const char* desc, void* data);
	static std::string getOfflineInfoExt(TIMMessageHandle  handle);
	static void getWBDataFromIM(TIMMessageHandle  handle, int elemCount, bool gzip, TIMManager* pIMService);
	static void getMsgFromIM(TIMMessageHandle  handle, int elemCount, TIMManager* pIMService);

	IMSendMsgCallBack * m_sendMsgCallback = nullptr;
	IMSendWBDataCallBack * m_sendWBDataCallback = nullptr;
	IMRecvMsgCallback * m_recvMsgCallback = nullptr;
	IMRecvWBDataCallback * m_recvWBDataCallback = nullptr;
	IMGroupChangeCallback * m_groupChangeCallback = nullptr;
	unsigned int m_seq = 1;
	std::string m_msgHead;
};