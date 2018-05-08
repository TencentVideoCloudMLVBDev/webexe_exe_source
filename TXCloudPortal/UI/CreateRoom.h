#pragma once

#include <QDialog>

#include "ui_CreateRoom.h"

typedef std::function<void(bool, const std::string&)> handlefunction;

class CreateRoom : public QDialog
{
    Q_OBJECT

public:
    CreateRoom(QWidget *parent = Q_NULLPTR);
    ~CreateRoom();

    void setLogo(QString styleSheet);

    void setHanleFunction(handlefunction func);
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
protected slots:
    void onCloseBtnClicked();
    void onOperateBtnClicked();
    void onCancelBtnClicked();
private:
    Ui::CreateRoom ui;
    bool m_pressed;
    QPoint m_point;

    handlefunction m_func;
    QString m_roomName;
};
