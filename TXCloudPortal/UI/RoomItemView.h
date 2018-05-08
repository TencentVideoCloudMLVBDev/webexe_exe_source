#pragma once

#include <QWidget>
#include "ui_RoomItemView.h"

class RoomItemView : public QWidget
{
    Q_OBJECT

public:
    RoomItemView(QWidget *parent = Q_NULLPTR);
    ~RoomItemView();

    QString roomID() const;
    void setRoomID(const QString& id);

    QString roomName() const;
    void setRoomName(const QString& info);

    int memberNum() const;
    void setMemberNum(int num);
private:
    Ui::RoomItemView ui;

    int m_memberNum;
};
