#pragma once

#include <QtWidgets/QDialog>
#include "ui_DialogMessage.h"

// 示例
// DialogMessage::exec("腾讯云教育白板", EduMessageBox::OK | EduMessageBox::CANCEL);

class DialogMessage : public QDialog
{
    Q_OBJECT

public:
    enum StandardButton
    {
        OK = 1,
        CANCEL = 2,
    };

    enum ResultCode
    {
        Rejected,
        Accepted,
        Timeout,
    };

    DialogMessage(QWidget *parent = Q_NULLPTR);
    ~DialogMessage();

    void setTitle(const QString& title);
    void setDelayClose(int delayMs);

    static DialogMessage::ResultCode exec(const QString& title, int btns);
    static DialogMessage::ResultCode exec(const QString& title, int btns, int delayCloseMs);
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void timerEvent(QTimerEvent *event);

protected slots:
    void on_btn_close_clicked();
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::DialogMessage ui;

    bool m_pressed;
    QPoint m_point;
    int m_timerID;
};
