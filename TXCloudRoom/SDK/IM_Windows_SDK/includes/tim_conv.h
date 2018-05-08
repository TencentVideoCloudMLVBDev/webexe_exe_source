#ifndef __TIM_IMPL_H__
#define __TIM_IMPL_H__

#include <vector>
#include "tim_comm.h"

namespace imcore {

class TIM_DECL TIMMessage;

class TIM_DECL TIMDraft;

class TIM_DECL TIMMessageOptResult; 

class TIM_DECL TIMConversation {
public:
	TIMConversation() {}
	void SendMsg(TIMMessage &msg, TIMCallBack *cb);
	void SendRedPacketMsg(TIMMessage &msg, TIMCallBack *cb);
	void SendLikeMsg(TIMMessage &msg, TIMCallBack *cb);
	void SendOnlineMsg(TIMMessage &msg, TIMCallBack *cb);
	int  SaveMsg(TIMMessage &msg, const std::string& sender, bool readed);
	int	ImportMsgs(std::vector<TIMMessage>& msgs);
	void GetMsgs(int count, const TIMMessage *last_msg, TIMValueCallBack<const std::vector<TIMMessage> &> *cb);
	std::vector<TIMMessage> GetMsgsFromCache(int count, const TIMMessage* last_msg);
	void GetMsgsForward(int count, const TIMMessage *from_msg, TIMValueCallBack<const std::vector<TIMMessage> &> *cb);
	void GetLocalMsgs(int count, const TIMMessage *last_msg, TIMValueCallBack<const std::vector<TIMMessage> &> *cb);
	void DeleteLocalMsg(TIMCallBack *cb);
	void SetReadMsg(const TIMMessage &last_read_msg);
	void SetReadMsg();
	uint64_t GetUnreadMessageNum();
	void DisableStorage();
	int ReplaceMessage(TIMMessage &msg, std::vector<TIMMessage> &msgs);

	const std::string &peer() const;
	const TIMConversationType &type() const;
	void set_peer(const std::string &peer);
	void set_type(const TIMConversationType &type);

	void SetDraft(const TIMDraft& draft);
	TIMDraft GetDraft();
	bool HasDraft() const;
	void FindMessages(TIMMessageLocator* pLocatorArray, int arrSize,TIMValueCallBack<const std::vector<TIMMessage> &> *cb);

	int DeleteRambleMessage(std::vector<TIMMessage> &msgs, TIMValueCallBack<const std::vector<TIMMessageOptResult> &> *cb);

private:
	TIMConversationType type_;
	std::string peer_;

friend class TIMMessageImpl;
friend class TIMManager;
};

}

#endif
