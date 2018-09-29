#pragma once
#include "TXCCosHelper.h"
#include "BoardSDK.h"
#include "TIMManager.h"
#include "commonType.h"
#include "json.h"
#include "HttpReportRequest.h"

struct BoardServiceCallback
{
	virtual void onUploadProgress(int percent) = 0;
	virtual void onUploadResult(bool success, bool openHistory, const std::wstring& objName) = 0;
	virtual void onStatusChanged(bool canUndo, bool canRedo, bool canCopy, bool canRemove) = 0;
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

    void openHistoryFile(const std::wstring& objName);

	uint32_t getPageIndex() const;

	uint32_t getPageCount() const;

	void gotoPage(uint32_t pageIndex);

	void gotoLastPage();

	void gotoNextPage();

	void insertPage();

	void deletePage();

	void init(const BoardAuthData& authData, const std::string& ip, unsigned short port);

	void setRoomID(const std::string& roomID);

	void gotoCurrentPage();

	void reportELK();

private:
	void sendUploadProgress(int percent) const;

	void sendUploadResult(bool success, bool openHistory, const std::wstring& objName) const;

	void sendStatusChanged() const;

	void sendSyncEventResult(bool success) const;

	std::wstring url() const;

	void fetchCosSig(const std::wstring& fileName);

	void uploadToCos(const std::string& sig, const std::wstring& fileName);

	void previewFile(const std::wstring& objName, bool openHistory);

	void reportEvent(Json::Value event);

	void syncEvent();

private:
	void onActionsData(const char* data, uint32_t length) override;
	void onStatusChanged(bool canUndo, bool canRedo, bool canCopy, bool canRemove) override;
	uint32_t onGetTime() override;

	void onRecvWhiteBoardData(const char* data, uint32_t length) override;
	void onGetBoardData(bool bResult) override;
	void onRenderFrame(bool render) override;
	void onReportBoardData(const int code, const char * msg) override;
private:
	BoardService();
	~BoardService();

private:
	BoardServiceCallback* _callback{nullptr};

	bool _canUndo{ false };
	bool _canRedo{ false };
	bool _canCopy{ false };
	bool _canRemove{ false };
	uint32_t _dataSeq{ 0 };

	BoardSDK* _board;

	TXCCosHelper _cos;
	std::string _cosSign;

	uint32_t _pageIndex{0};
	std::vector<std::string> _pagesId;
	std::vector<std::wstring> _backsUrl;
	std::vector<bool> _backsSend;

	BoardAuthData m_authData;
    std::string m_roomID;
	bool m_bUpload = false;
	bool m_bLast = false;
	bool m_bNext = false;
    bool m_bNewUpload = false;
    int m_tryPreview = 3;
};
