#include "RoomItemView.h"

RoomItemView::RoomItemView(QWidget *parent)
    : QWidget(parent)
    , m_memberNum(0)
{
    ui.setupUi(this);
}

RoomItemView::~RoomItemView()
{
}

QString RoomItemView::roomID() const
{
    return ui.label_room_id->text();
}

void RoomItemView::setRoomID(const QString& id)
{
    ui.label_room_id->setText(id);
}

QString RoomItemView::roomName() const
{
    return ui.label_room_name->text();
}

void RoomItemView::setRoomName(const QString& info)
{
    ui.label_room_name->setText(info);
}

int RoomItemView::memberNum() const
{
    return m_memberNum;
}

void RoomItemView::setMemberNum(int num)
{
    m_memberNum = num;
}
