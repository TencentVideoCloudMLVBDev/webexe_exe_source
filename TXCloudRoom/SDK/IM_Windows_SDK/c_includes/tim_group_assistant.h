#ifndef _TIM_GROUP_ASSISTANT_H_
#define _TIM_GROUP_ASSISTANT_H_

#include "tim_comm_c.h"
#include "tim_group_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*CBOnMemberJoin)(const char* groupId, const char** id, uint32_t num);
typedef void(*CBOnMemberQuit)(const char* groupId, const char** id, uint32_t num);
typedef void(*CBOnMemberUpdate)(const char* groupId, const char** id, uint32_t num);

//group info notify
typedef void(*CBOnGroupAdd)(const char* groupId);
typedef void(*CBOnGroupDelete)(const char* groupId);
typedef void(*CBOnGroupUpdate)(const char* groupId);

typedef struct _T_TIMGroupAssistantCallBack
{
	CBOnMemberJoin onMemberJoin;	//有新用户加入群时的通知回调
	CBOnMemberQuit onMemberQuit;	//有群成员退群时的通知回调
	CBOnMemberUpdate onMemberUpdate;//群成员信息更新的通知回调
	CBOnGroupAdd onAdd;				//加入群的通知回调
	CBOnGroupDelete onQuit;			//解散群的通知回调
	CBOnGroupUpdate onUpdate;		//群资料更新的通知回调
}TIMGroupAssistantCallBack;

struct TIMUpdateInfoOpt{
	uint64_t flag;
	const char** tag_name;
	uint32_t num;
};

struct TIMGroupSettings{
	TIMUpdateInfoOpt memberInfoOpt;
	TIMUpdateInfoOpt groupInfoOpt;
};

/**
Description:	设置拉取字段
@param	[in]	config	拉取设置
@return			void
@exception      none
*/
TIM_DECL void TIMInitGroupSetting(TIMGroupSettings* config);
/**
Description:	启用群资料存储
@return			void
@exception      none
*/
TIM_DECL void TIMEnableGroupAssistantStorage();
/**
Description:	群通知回调
@param	[in]	cb		回调
@return			void
@exception      none
*/
TIM_DECL void TIMSetGroupAssistantCallBack(TIMGroupAssistantCallBack* cb);

typedef void* TIMGroupAssistantGroupInfo;
TIM_DECL TIMGroupAssistantGroupInfo CreateTIMGroupAssistantGroupInfo();
TIM_DECL void DestroyTIMGroupAssistantGroupInfo(TIMGroupAssistantGroupInfo info);

TIM_DECL uint32_t TIMGetGroupNumber4TIMGroupAssistantGroupInfo(TIMGroupAssistantGroupInfo info);
TIM_DECL TIMGroupDetailInfoHandle TIMGetGroupDetailInfo4TIMGroupAssistantGroupInfo(TIMGroupAssistantGroupInfo info, uint32_t index);
/**
Description:	同步获取群信息
@param	[in]	info	同步信息
@return			void
@exception      none
*/
TIM_DECL void TIMGroupAssistantGetGroups(TIMGroupAssistantGroupInfo info);

#ifdef __cplusplus
}
#endif

#endif

