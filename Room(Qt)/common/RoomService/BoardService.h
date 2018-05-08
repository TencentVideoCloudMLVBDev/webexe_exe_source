#pragma once
#include "TXCCosHelper.h"
#include "BoardSDK.h"
#include "TIMManager.h"
#include "commonType.h"
#include "json.h"

struct BoardServiceCallback
{
	virtual void onUploadProgress(int percent) = 0;
	virtual void onUploadResult(bool success) = 0;
	virtual void onStatusChanged(bool canUndo, bool canRedo) = 0;
	virtual void onSyncEventResult(bool success) = 0;
};

class BoardService : public BoardCallback, public IMRecvWBDataCallback
{
public:
	static BoardService& instance();

	void setCallback(BoardServiceCallback* callback);

	HWND getRenderWindow() const;

	void appendActionsData(std::string& data) const;

	void appendEventData(std::string& data) const;

	void useTool(BoardTool tool) const;

	void setWidth(uint32_t width) const;

	void setColor(uint32_t rgba) const;

	void setFill(bool fill) const;

	void undo() const;

	void redo() const;

	void clear() const;

	void syncEventData();

	void uploadFile(const std::wstring& fileName);

	uint32_t getPageIndex() const;

	uint32_t getPageCount() const;

	void gotoPage(uint32_t pageIndex);

	void gotoLastPage();

	void gotoNextPage();

	void insertPage();

	void deletePage();

	void init(const BoardAuthData& authData);

	void setRoomID(const std::string& roomID);

	void gotoCurrentPage();

private:
	void sendUploadProgress(int percent) const;

	void sendUploadResult(bool success) const;

	void sendStatusChanged() const;

	void sendSyncEventResult(bool success) const;

	std::wstring url() const;

	void fetchCosSig(const std::wstring& fileName);

	void uploadToCos(const std::string& sig, const std::wstring& fileName);

	void previewFile(const std::wstring& objName);

	void reportEvent(Json::Value event);

	void syncEvent();

private:
	void onActionsData(const char* data, uint32_t length) override;
	void onBoardEventData(const char* data, uint32_t length) override;
	void onStatusChanged(bool canUndo, bool canRedo) override;
	uint32_t onGetTime() override;

	void onRecvWhiteBoardData(const char* data, uint32_t length) override;

private:
	BoardService();
	~BoardService();

private:
	BoardServiceCallback* _callback{nullptr};

	bool _canUndo{ false };
	bool _canRedo{ false };
	uint32_t _dataSeq{ 0 };

	BoardSDK* _board;

	TXCCosHelper _cos;

	uint32_t _pageIndex{0};
	std::vector<std::string> _pagesId;
	std::vector<std::wstring> _backsUrl;
	std::vector<bool> _backsSend;

	BoardAuthData m_authData;
    std::string m_roomID;
};
