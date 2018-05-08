// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 EDUSDK_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// EDUSDK_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#pragma once

#ifdef EDUSDK_EXPORTS
#define EDUSDK_API __declspec(dllexport)
#else
#define EDUSDK_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <functional>

class BoardSDKImpl;

/**
 * \brief 白板回调接口，用户需自行实现这个接口并调用BoardSDK的setCallback方法将该通道设置给白板
 */
struct BoardCallback
{
	/**
	 * \brief 操作事件数据到达，当使用者在白板上执行操作时，SDK将操作的元数据按如下JSON格式打包，并触发该函数
	 * {
	 *		"boardId":"#DEFAULT", 	// 白板id，默认为#DEFAULT
	 *		"operator":"user1",
	 *		"actions":
	 *			[
	 *				...	//一系列白板操作
	 *			],
	 *	}
	 * \param data		保存有数据的字符串缓冲区
	 * \param length	字符串长度
	 */
	virtual void onActionsData(const char *data, uint32_t length) = 0;

	/**
	 * \brief 白板事件数据到达，当白板页面或图层样式发生变化时，触发该函数，数据格式遵照协议
	 * \param data		保存有数据的字符串缓冲区
	 * \param length	字符串长度
	 */
	virtual void onBoardEventData(const char *data, uint32_t length) = 0;

	/**
	 * \brief 状态事件，当白板状态发生变化时触发
	 * \param canUndo	是否可撤销
	 * \param canRedo	是否可重做
	 */
	virtual void onStatusChanged(bool canUndo, bool canRedo) = 0;

	/**
	 * \brief 获取时间戳事件，SDK需要获取统一的时间戳时触发该事件
	 * \return 时间戳，单位毫秒
	 */
	virtual uint32_t onGetTime() = 0;
};

/**
 * \brief 白板工具定义
 */
enum class BoardTool
{
	None,			//不使用任何工具（禁止绘制）
	Pen,			//铅笔工具
	Eraser,			//橡皮擦工具
	Laser,			//激光笔
	Line,			//直线工具
	Ellipse,		//椭圆工具
	Rectangle,		//圆角矩形工具
	Select,			//选框工具
};

// 此类是从 BoardSDK.dll 导出的
class EDUSDK_API BoardSDK {
public:
	/**
	 * \brief 白板SDK构造函数
	 * \param userID				当前用户ID
	 * \param parentHWnd			父窗口句柄，为空则白板没有父窗口
	 */
	BoardSDK(const char *userID, HWND parentHWnd = nullptr);
	~BoardSDK();

	/**
	 * \brief 获取白板渲染窗口句柄
	 * \return 渲染窗口句柄
	 */
	HWND getRenderWindow() const;

	/**
	 * \brief 设置回调接口
	 * \param callback				回调接口
	 */
	void setCallback(BoardCallback *callback) const;

	/**
	 * \brief 白板页面操作
	 * \param toPageID				要跳转到页面ID
	 * \param deletePagesID			要删除的页面ID集合
	 * \param deletePagesCount		要删除的页面ID个数
	 */
	void pageOperate(const char *toPageID, const char **deletePagesID = nullptr, uint32_t deletePagesCount = 0) const;

	/**
	 * \brief 获取白板页面
	 * \param pagesID				字符串数组缓冲区用于接收页面ID，为空表示只查询白板页数（返回的白板ID顺序为随机排列）
	 * \param capacity				字符串数组长度
	 * \return 白板页数
	 */
	uint32_t getPages(char **pagesID = nullptr, uint32_t capacity = 0) const;

	/**
	 * \brief 指定要使用的白板工具
	 * \param tool					白板工具
	 */
	void useTool(BoardTool tool) const;

	/**
	 * \brief 指定要使用的宽度（对于铅笔、直线、椭圆、矩形工具，该参数指示了线宽；对于橡皮擦及激光笔工具，该参数指示了直径）
	 * \param width					宽度
	 */
	void setWidth(uint32_t width) const;

	/**
	 * \brief 指定要使用的颜色
	 * \param rgba					颜色值，按字节序从高到低分别为Red、Green、Blue、Alpha分量
	 */
	void setColor(uint32_t rgba) const;

	/**
	 * \brief 指定是否启用填充
	 * \param fill					是否启用填充
	 */
	void setFill(bool fill) const;

	/**
	 * \brief 指定要使用的半径（当前仅用于指定圆角矩形的圆角半径）
	 * \param radius				半径
	 */
	void setRadius(uint32_t radius) const;

	/**
	 * \brief 指定要使用的背景图片
	 * \param url					背景图片URL（本地文件以file://开头表示，为空表示清空背景）
	 * \param pageID				制定要设置背景图片的页面ID（为空表示当前页面）
	 */
	void useBackground(const wchar_t *url, const char *pageID = nullptr) const;

	/**
	 * \brief 对已选中图形进行拷贝操作
	 */
	void copy() const;

	/**
	 * \brief 对已选中图形进行删除操作
	 */
	void remove() const;

	/**
	 * \brief 撤销
	 */
	void undo() const;

	/**
	 * \brief 重做
	 */
	void redo() const;

	/**
	 * \brief 清空白板
	 */
	void clear() const;

	/**
	 * \brief 向白板添加操作元数据（通过BoardCallback的sendData方法获取的数据，按如下JSON格式打包）
	 * {
	 *		"boardId":"#DEFAULT", 	// 白板id，默认为#DEFAULT
	 *		"operator":"user1",
	 *		"actions":
	 *			[
	 *				...	//一系列白板操作
	 *			],
	 *	}
	 * \param data					保存有数据的字符串缓冲区
	 * \param length				字符串长度
	 */
	void appendActionsData(const char *data, uint32_t length) const;

	/**
	 * \brief 向白板添加事件数据，数据格式遵照协议
	 * \param data					保存有数据的字符串缓冲区
	 * \param length				字符串长度
	 */
	void appendBoardEventData(const char *data, uint32_t length) const;

	/**
	 * \brief 将函数委托到主线程消息队列中执行
	 * \param func					要执行的函数
	 * \return 委托是否成功
	 */
	bool runOnMsgQueue(const std::function<void()> func) const;

private:
	BoardSDKImpl *_impl;
};
