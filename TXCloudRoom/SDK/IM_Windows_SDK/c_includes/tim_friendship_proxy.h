#ifndef _TIM_FRIENDSHIP_PROXY_H_
#define _TIM_FRIENDSHIP_PROXY_H_

#include "tim_friend_c.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
* 好友代理状态
*/
typedef enum {
	/**
	*  未初始化
	*/
	TIM_FRIENDSHIP_STATUS_NONE = 0x0,
		/**
		*  正在和服务器同步数据
		*/
		TIM_FRIENDSHIP_STATUS_SYNCING = 0x1,
		/**
		*  已经完成数据同步
		*/
		TIM_FRIENDSHIP_STATUS_SYNCED = 0x2,
		/**
		*  数据同步失败
		*/
		TIM_FRIENDSHIP_STATUS_FAILED = 0x3,
}TIM_FRIENDSHIP_PROSY_STATUS;




typedef void(*CBOnProxyStatusChange) (TIM_FRIENDSHIP_PROSY_STATUS status);
typedef void(*CBOnAddFriends)(const char** id, uint32_t num);
typedef void(*CBOnDelFriends)(const char**id, uint32_t num);
typedef void(*CBOnFriendProfileUpdate)(const char** id, uint32_t num);


typedef struct 
{
	CBOnProxyStatusChange onProxyStatusChange;
	CBOnAddFriends onAddFriends;
	CBOnDelFriends onDelFriends;
	CBOnFriendProfileUpdate onFriendProfileUpdate;
}TIMFriendshipPorxyListener;

typedef struct _T_TIMFriendshipProxyConfig
{
	uint64_t flags;
	const char** key;
	uint32_t key_num;
}TIMFriendshipProxyConfig;

TIM_DECL void TIMInitFriendshipSetting(TIMFriendshipProxyConfig* config);
/**
Description:	设置好友代理回调
@param	[in]	listener	回调
@return			void
@exception      none
*/
TIM_DECL void TIMSetFriendshipProxyListener(TIMFriendshipPorxyListener* listener);
/**
Description:	开启存储
@return			void
@exception      none
*/
TIM_DECL void TIMEnableFriendshipProxy();

//sdk申请一段内存
/**
Description:	内存中同步获取关系链资料数据
@param	[in/out]	handles profile handle 数组
@param	[in/out]	num	获取个数
@return			int
@exception      none
*/
TIM_DECL int TIMFriendProxyGetFriendList(TIMProfileHandle* handles, uint32_t* num);


#ifdef __cplusplus
}
#endif

#endif