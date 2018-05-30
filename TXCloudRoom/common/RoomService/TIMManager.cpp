#include "TIMManager.h"
#include "json.h"
#include "gzip.h"

#include <chrono>

#define  WHITEBOARD_DATA_MAX 7000
#define  MAX_ID_LEN          100
#define  IM_TMP_BUF_SIZE     256

typedef struct tag_SendTIMMsgSink
{
    TIMConversationHandle  conv;
    IMSendMsgCallBack *    send_msg_sink;
} SendTIMMsgSink;

typedef struct tag_SendWBDataSink
{
    TIMConversationHandle  conv;
    IMSendWBDataCallBack * send_wbdata_sink;
} SendWBDataSink;

typedef struct tag_OptGroupSink
{
    std::string             group_id;
    IMGroupOptType          group_opt_type;
    IMGroupOptCallBack *   group_opt_sink;
} OptGroupSink;

std::string getMessageElemData(void * handle, uint32_t(*getDataLen)(void *), int(*getData)(void *, char *, uint32_t *))
{
	std::string buffer;
	buffer.resize(getDataLen(handle) + 32);

	uint32_t len = buffer.size();

	getData(handle, (char*)buffer.c_str(), &len);
	buffer.resize(len);

	return buffer;
}


void TIMManager::onTIMNewMessage(TIMMessageHandle* handles, uint32_t msg_num, void* data)
{
	TIMManager* pIMService = (TIMManager *)data;
	if (pIMService) 
	{
		for (int msg_idx = 0; msg_idx < msg_num; msg_idx++)
		{
			TIMMessageHandle handle = handles[msg_idx];
			int elemCount = GetElemCount(handle);

			std::string offline_info_ext = getOfflineInfoExt(handle);
			if (offline_info_ext.size() >= 15 && offline_info_ext.substr(0, 15) == "TXWhiteBoardExt")
			{
				if (!pIMService->m_recvWBDataCallback)
				{
					continue;
				}
				bool gzip = (offline_info_ext != "TXWhiteBoardExt");
				getWBDataFromIM(handle, elemCount, gzip, pIMService);
			}
			else
			{
				if (!pIMService->m_recvMsgCallback)
				{
					continue;
				}
				getMsgFromIM(handle, elemCount, pIMService);
			}
		}
	}
}

void TIMManager::onTIMKickOffline(void* data)
{
    TIMManager* pIMService = (TIMManager *)data;
    if (pIMService && pIMService->m_kickOfflineCallBack)
    {
        pIMService->m_kickOfflineCallBack->onKickOffline();
    }
}

void TIMManager::OnGetWBFileDataSuccess(const char * buf, uint32_t len, void * data)
{
	std::string str_data = gzip::decompress(std::string(buf, len));
	TIMManager* pIMService = reinterpret_cast<TIMManager*>(data);
	pIMService->onRecvWBData(str_data);
}

void TIMManager::OnGetWBFileDataError(int code, const char * desc, void * data)
{
}

void onSendTIMMsgSuccess(void* data)
{
	SendTIMMsgSink * sink = (SendTIMMsgSink *)data;
	if (sink != NULL)
	{
		if (sink->send_msg_sink != NULL) sink->send_msg_sink->onSendMsg(0, NULL);

		DestroyConversation(sink->conv);
		delete sink;
	}
}

void onSendTIMMsgError(int code, const char* desc, void* data)
{
	SendTIMMsgSink * sink = (SendTIMMsgSink *)data;
	if (sink != NULL)
	{
		if (sink->send_msg_sink != NULL) sink->send_msg_sink->onSendMsg(code, desc);

		DestroyConversation(sink->conv);
		delete sink;
	}
}

void OnSendWBDataSuccess(void* data)
{
	SendWBDataSink * sink = (SendWBDataSink *)data;
	if (sink != NULL)
	{
		if (sink->send_wbdata_sink != NULL) sink->send_wbdata_sink->onSendWBData(0, NULL);

		DestroyConversation(sink->conv);
		delete sink;
	}
}

void OnSendWBDataError(int code, const char* desc, void* data)
{
	SendWBDataSink * sink = (SendWBDataSink *)data;
	if (sink != NULL)
	{
		if (sink->send_wbdata_sink != NULL) sink->send_wbdata_sink->onSendWBData(code, desc);

		DestroyConversation(sink->conv);
		delete sink;
	}
}

TIMManager::TIMManager()
{ 

}

TIMManager::~TIMManager()
{
	TIMUninit();
}

TIMManager * TIMManager::instance()
{
	static TIMManager * pIMService = new TIMManager;
	return pIMService;
}

void TIMManager::init(const std::string& ip, unsigned short port)
{
    TIMSetMode(1);
    TIMDisableStorage();
    TIMInit();
	TIMSetProxy(ip.c_str(), static_cast<int>(port));
}

TIMForceOfflineCB g_forceOfflineCB = { 0 };

void TIMManager::login(const IMAccountInfo & IMInfo, TIMCommCB *callback)
{
	TIMUserInfo user = { 0 };
	user.account_type = const_cast<char*>(IMInfo.accType.c_str());
	std::string strAppID = std::to_string(IMInfo.sdkAppID);
	user.app_id_at_3rd = const_cast<char*>(strAppID.c_str());
	user.identifier = const_cast<char*>(IMInfo.userID.c_str());

	TIMLogin(IMInfo.sdkAppID, &user, IMInfo.userSig.c_str(), callback);

    TIMMessageCB cb = { 0 };
    cb.data = this;
    cb.OnNewMessage = onTIMNewMessage;
    TIMSetMessageCallBack(&cb);

    g_forceOfflineCB.data = this;
    g_forceOfflineCB.OnKickOffline = onTIMKickOffline;
    TIMSetKickOfflineCallBack(&g_forceOfflineCB);
}

void onlogoutSuss(void *)
{
}

void onlogoutErr(int, const char *, void *)
{
}

void TIMManager::logout()
{
    TIMCommCB logoutCB = { 0 };
	logoutCB.data = 0;
	logoutCB.OnError = onlogoutErr;
	logoutCB.OnSuccess = onlogoutSuss;
	TIMLogout(&logoutCB);
}

void TIMManager::setMsgHead(const char * msgHead)
{
	m_msgHead = msgHead;
}

void onGroupOptSuccess(void * data)
{
    OptGroupSink * optGroupSink = (OptGroupSink*)data;
    if (optGroupSink)
    {
        if (optGroupSink->group_opt_sink)
        {
            optGroupSink->group_opt_sink->onGroupOptSuccess(optGroupSink->group_opt_type, optGroupSink->group_id.c_str());
        }

        delete optGroupSink;
    }
}

void onGroupOptError(int code, const char* desc, void * data)
{
    OptGroupSink * optGroupSink = (OptGroupSink*)data;
    if (optGroupSink)
    {
        if (optGroupSink->group_opt_sink)
        {
            optGroupSink->group_opt_sink->onGroupOptError(optGroupSink->group_opt_type, optGroupSink->group_id.c_str()
            , code, desc);
        }

        delete optGroupSink;
    }
}

void TIMManager::opGroup(IMGroupOptType opType, const char * group_id, IMGroupOptCallBack * sink)
{
	OptGroupSink * optGroupSink = new OptGroupSink();
	optGroupSink->group_id = group_id;
	optGroupSink->group_opt_type = opType;
	optGroupSink->group_opt_sink = sink;

	TIMCommCB cb = { 0 };
	cb.data = optGroupSink;
	cb.OnSuccess = onGroupOptSuccess;
	cb.OnError = onGroupOptError;

	switch (opType)
	{
	case kgJoinGroup:
		TIMApplyJoinGroup(group_id, "", &cb);
		break;
	case kgQuitGroup:
		TIMQuitGroup(group_id, &cb);
		break;
	case kgDeleteGroup:
		TIMDeleteGroup(group_id, &cb);
		break;
	default:
		break;
	}
}

void TIMManager::setRecvMsgCallBack(IMRecvMsgCallback * sink)
{
	m_recvMsgCallback = sink;
}

void TIMManager::setRecvWBDataCallBack(IMRecvWBDataCallback * sink)
{
	m_recvWBDataCallback = sink;
}

void TIMManager::setSendMsgCallBack(IMSendMsgCallBack * sink)
{
	m_sendMsgCallback = sink;
}

void TIMManager::setSendWBDataCallBack(IMSendWBDataCallBack * sink)
{
	m_sendWBDataCallback = sink;
}

void TIMManager::setGroupChangeCallBack(IMGroupChangeCallback * sink)
{
	m_groupChangeCallback = sink;
}

void TIMManager::setKickOfflineCallBack(IMKickOfflineCallBack * sink)
{
    m_kickOfflineCallBack = sink;
}

void TIMManager::sendC2CTextMsg(const char * dstUser, const char * msg)
{
	TIMMessageHandle IMMsg = CreateTIMMessage();
	TIMMsgTextElemHandle elem = CreateMsgTextElem();

	SetContent(elem, msg);
	AddElem(IMMsg, elem);

	int ret = sendMsg(kCnvC2C, dstUser, IMMsg);

	DestroyElem(elem);
	DestroyTIMMessage(IMMsg);
}

void TIMManager::sendGroupTextMsg(const char* groupId, const char * msg)
{
	TIMMessageHandle IMMsg = CreateTIMMessage();
	TIMMsgTextElemHandle elem = CreateMsgTextElem();
	TIMMsgTextElemHandle elemHead;
	if (!m_msgHead.empty())
	{
		elemHead = CreateCustomElemHandle();
		SetCustomElemData(elemHead, m_msgHead.c_str(), strlen(m_msgHead.c_str()));
		AddElem(IMMsg, elemHead);
	}

	SetContent(elem, msg);
	AddElem(IMMsg, elem);

	int ret = sendMsg(kCnvGroup, groupId, IMMsg);

	DestroyElem(elem);
	DestroyTIMMessage(IMMsg);

	if (!m_msgHead.empty())
	{
		DestroyElem(elemHead);
	}
}

void TIMManager::sendC2CCustomMsg(const char * dstUser, const char * msg)
{
	TIMMessageHandle IMMsg = CreateTIMMessage();
	TIMMsgTextElemHandle elem = CreateCustomElemHandle();

	SetCustomElemData(elem, msg, strlen(msg));
	AddElem(IMMsg, elem);

	int ret = sendMsg(kCnvC2C, dstUser, IMMsg);

	DestroyElem(elem);
	DestroyTIMMessage(IMMsg);
}

void TIMManager::sendGroupCustomMsg(const char* groupId, const char * msg)
{
	TIMMessageHandle IMMsg = CreateTIMMessage();
	TIMMsgTextElemHandle elem = CreateCustomElemHandle();
	TIMMsgTextElemHandle elemHead;
	if (!m_msgHead.empty())
	{
		elemHead = CreateCustomElemHandle();
		SetCustomElemData(elemHead, m_msgHead.c_str(), strlen(m_msgHead.c_str()));
		AddElem(IMMsg, elemHead);
	}

	SetCustomElemData(elem, msg, strlen(msg));
	AddElem(IMMsg, elem);

	int ret = sendMsg(kCnvGroup, groupId, IMMsg);

	DestroyElem(elem);
	DestroyTIMMessage(IMMsg);
}

void TIMManager::sendWhiteBoardData(const char* groupId, const char * data, uint32_t length)
{
	Json::Value value;
	Json::Reader reader;
	if (!reader.parse(data, value))
	{
		return;
	}

	std::chrono::milliseconds currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	Json::Value json;
	json["seq"] = m_seq++;
	json["timestamp"] = static_cast<unsigned>(currentTime.count());
	json["value"] = value;
	Json::FastWriter writer;
	std::string sendData = writer.write(json);

	TIMConversationHandle conv = CreateConversation();
	TIMGetConversation(conv, kCnvGroup, groupId);
	TIMMessageHandle msg = CreateTIMMessage();

	std::string infoStr = "TXWhiteBoardExt";

	if (sendData.size() > WHITEBOARD_DATA_MAX)
	{
		infoStr = "TXWhiteBoardExt_gzip_" + std::to_string(sendData.size());
		sendData = gzip::compress(sendData);
	}

	if (sendData.size() <= WHITEBOARD_DATA_MAX)
	{
		TIMMsgCustomElemHandle elem = CreateCustomElemHandle();
		SetCustomElemData(elem, sendData.c_str(), sendData.size());
		SetCustomElemExt(elem, infoStr.c_str(), infoStr.size());
		AddElem(msg, elem);
	}
	else
	{
		TIMMsgFileElemHandle elem = CreateFileElemHandle();
		SetFileElemData(elem, sendData.c_str(), sendData.size());
		SetFileElemFileName(elem, infoStr.c_str(), infoStr.size());
		AddElem(msg, elem);
	}

	TIMOfflineInfoHandle offline_info = CreateTIMTIMOfflineInfo();
	SetExt4TIMTIMOfflineInfo(offline_info, infoStr.c_str(), infoStr.size());
	SetMsgOfflinePushInfo(msg, offline_info);

	SendWBDataSink * cbsink = new SendWBDataSink();
	cbsink->conv = conv;
	cbsink->send_wbdata_sink = m_sendWBDataCallback;

	TIMCommCB cb;
	cb.data = cbsink;
	cb.OnSuccess = OnSendWBDataSuccess;
	cb.OnError = OnSendWBDataError;
	SendMsg(conv, msg, &cb);
}

int TIMManager::sendMsg(TIMConversationType type, const char * peer, TIMConversationHandle msg)
{
	TIMConversationHandle conv = CreateConversation();

	int iRet = TIMGetConversation(conv, type, peer);

	if (iRet != 0)
	{
		DestroyConversation(conv);
		return iRet;
	}

	SendTIMMsgSink * cbsink = new SendTIMMsgSink();
	cbsink->conv = conv;
	cbsink->send_msg_sink = m_sendMsgCallback;

	TIMCommCB cb = { 0 };
	cb.data = cbsink;
	cb.OnSuccess = onSendTIMMsgSuccess;
	cb.OnError = onSendTIMMsgError;

	SendMsg(conv, msg, &cb);

	return 0;
}

void TIMManager::onRecvWBData(const std::string& data)
{
	Json::Reader reader;
	Json::FastWriter writer;
	Json::Value json;
	if (reader.parse(data, json))
	{
		const Json::Value value = json["value"];
		const std::string str_data = writer.write(value);
		if(m_recvWBDataCallback)
		{
			m_recvWBDataCallback->onRecvWhiteBoardData(str_data.c_str(), str_data.size());
		}
	}
}

std::string TIMManager::getOfflineInfoExt(TIMMessageHandle handle)
{
	TIMOfflineInfoHandle offline_info = CreateTIMTIMOfflineInfo();
	GetMsgOfflinePushInfo(handle, offline_info);

	char tmp_buf[IM_TMP_BUF_SIZE] = { 0 };
	uint32_t len = IM_TMP_BUF_SIZE;
	GetExt4TIMTIMOfflineInfo(offline_info, tmp_buf, &len);
	std::string offline_info_ext(tmp_buf);
	return offline_info_ext;
}

void TIMManager::getWBDataFromIM(TIMMessageHandle handle, int elemCount, bool gzip, TIMManager* pIMService)
{
	for (int i = 0; i < elemCount; i++)
	{
		auto elem = GetElem(handle, i);
		auto type = GetElemType(elem);
		std::string str_data;
		if (type == kElemCustom) {
			str_data = getMessageElemData(elem, GetCustomElemDataLen, GetCustomElemData);

			if (gzip)
			{
				str_data = gzip::decompress(str_data);
			}
			pIMService->onRecvWBData(str_data);
		}
		else if (type == kElemFile)
		{
			TIMGetMsgDataCB cb;
			cb.OnSuccess = OnGetWBFileDataSuccess;
			cb.OnError = OnGetWBFileDataError;
			cb.data = pIMService;
			GetFileFromFileElem(elem, &cb);
		}
	}
}

void TIMManager::getMsgFromIM(TIMMessageHandle handle, int elemCount, TIMManager * pIMService)
{
	char group_id[MAX_ID_LEN] = { 0 };
	uint32_t group_id_len = MAX_ID_LEN - 1;
	char user_id[MAX_ID_LEN] = { 0 };
	uint32_t user_id_len = MAX_ID_LEN - 1;

	TIMConversationHandle conv = CreateConversation();

	GetMsgSender(handle, user_id, &user_id_len);
	user_id[user_id_len] = 0;

	GetConversationFromMsg(conv, handle);
	std::string msgHead = "";

	for (int i = 0; i < elemCount; i++)
	{
		auto elem = GetElem(handle, i);
		auto type = GetElemType(elem);
		bool bRoomMessage = true;

		switch (type)
		{
		case kElemText:
		{
			std::string text_msg = getMessageElemData(elem, GetContentLen, GetContent);

			TIMConversationType convType = GetConversationType(conv);
			group_id[0] = 0;
			if (convType == kCnvGroup)
			{
				GetConversationPeer(conv, group_id, &group_id_len);
				group_id[group_id_len] = 0;

				if (bRoomMessage && elemCount == 2)
					pIMService->m_recvMsgCallback->onRecvGroupTextMsg(group_id, user_id, text_msg.c_str(), msgHead.c_str());
				else
					pIMService->m_recvMsgCallback->onRecvGroupTextMsg(group_id, user_id, text_msg.c_str());
			}
			else if (convType == kCnvC2C)
			{
				pIMService->m_recvMsgCallback->onRecvC2CTextMsg(user_id, text_msg.c_str());
			}
		}
			break;
		case kElemCustom:
		{
			std::string text_msg = getMessageElemData(elem, GetCustomElemDataLen, GetCustomElemData);

			TIMConversationType convType = GetConversationType(conv);
			group_id[0] = 0;
			if (bRoomMessage && elemCount == 2)
			{
				msgHead = text_msg;
			}
			else
			{
				if (convType == kCnvGroup)
				{
					GetConversationPeer(conv, group_id, &group_id_len);
					group_id[group_id_len] = 0;
					pIMService->m_recvMsgCallback->onRecvGroupCustomMsg(group_id, user_id, text_msg.c_str());
				}
				else if (convType == kCnvC2C)
				{
					pIMService->m_recvMsgCallback->onRecvC2CCustomMsg(user_id, text_msg.c_str());
				}
			}
		}
			break;
		case kElemGroupReport:
		{
			E_TIM_GROUP_SYSTEM_TYPE sys_type = GetGroupReportType(elem);
			if (sys_type == TIM_GROUP_SYSTEM_CUSTOM_INFO)
			{
				GetGroupReportID(elem, group_id, &group_id_len);
				group_id[group_id_len] = 0;

				std::string text_msg = getMessageElemData(elem, GetGroupReportUserDataLen, GetGroupReportUserData);
				
				pIMService->m_recvMsgCallback->onRecvGroupCustomMsg(group_id,user_id, text_msg.c_str());
			}
			else if (sys_type == TIM_GROUP_SYSTEM_DELETE_GROUP_TYPE)
			{
				GetGroupReportID(elem, group_id, &group_id_len);
				group_id[group_id_len] = 0;
                if (pIMService->m_groupChangeCallback)
                {
                    pIMService->m_groupChangeCallback->onGroupChangeMessage(kgDeleteGroup, group_id, "");
                }
			}
			else if (sys_type == TIM_GROUP_SYSTEM_CREATE_GROUP_TYPE)
			{
				GetGroupReportID(elem, group_id, &group_id_len);
				group_id[group_id_len] = 0;
                if (pIMService->m_groupChangeCallback)
                {
                    pIMService->m_groupChangeCallback->onGroupChangeMessage(kgCreateGroup, group_id, "");
                }
			}
		}
			break;
		default:
			break;
		}
	}
	DestroyConversation(conv);
}
