#pragma once

#include <QDialog>
#include "ui_RTCRoomList.h"
#include "HttpRequest.h"
#include "CreateRoom.h"

typedef std::function<void(void)> RTCRoomListfunction;

class RTCRoomList : public QDialog
{
    Q_OBJECT

public:
    RTCRoomList(QWidget *parent = Q_NULLPTR);
    ~RTCRoomList();

    void setMulti(bool multi);
signals:
    void dispatch(RTCRoomListfunction func);     // 投递线程队列
protected slots:
    void handle(RTCRoomListfunction func);       // 执行函数

    void onCloseBtnClicked();
    void onJoinBtnClicked();
    void onCreateBtnClicked();
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void timerEvent(QTimerEvent *event);

    static void CALLBACK onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

    void getLoginInfo(const std::string& userID);
    void createRoom(const std::string& roomID, const std::string& roomInfo, RoomType roomType);
    void getRoomList(int index, int cnt);
	void getConfigInfo(std::string& title, std::string& logo, std::string& domain);
private:
    Ui::RTCRoomList m_ui;
    bool m_pressed;
    QPoint m_point;

    CreateRoom* m_createRoom;

    bool m_multi;
    AuthData m_authData;
    HttpRequest m_httpRequest;
    std::string m_roomID;
    RoomType m_roomType;

    int m_listTimerID;
    MMRESULT m_hearbeatTimerID;
};
