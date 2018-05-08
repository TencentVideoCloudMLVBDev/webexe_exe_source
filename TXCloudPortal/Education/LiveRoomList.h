#pragma once

#include <QDialog>

#include "ui_LiveRoomList.h"
#include "CreateRoom.h"
#include "HttpRequest.h"

typedef std::function<void(void)> LiveRoomListfunction;

class LiveRoomList : public QDialog
{
    Q_OBJECT

public:
    LiveRoomList(QWidget *parent = Q_NULLPTR);
    ~LiveRoomList();
signals:
    void dispatch(LiveRoomListfunction func);     // 投递线程队列
    protected slots:
    void handle(LiveRoomListfunction func);       // 执行函数

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
private:
    Ui::LiveRoomList m_ui;
    bool m_pressed;
    QPoint m_point;

    CreateRoom* m_createRoom;

    AuthData m_authData;
    HttpRequest m_httpRequest;
    std::string m_roomID;

    int m_listTimerID;
    MMRESULT m_hearbeatTimerID;
};
