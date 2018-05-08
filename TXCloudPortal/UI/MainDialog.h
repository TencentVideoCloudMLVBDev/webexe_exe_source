#pragma once

#include <QDialog>

#include "ui_MainDialog.h"
#include "Education/LiveRoomList.h"
#include "CourtCS/RTCRoomList.h"
#include "CSLive/CSLiveSetting.h"

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = Q_NULLPTR);
    ~MainDialog();
private:
    QPoint mousePressedPosition; // 鼠标按下时的坐标
    QPoint windowPositionAsDrag; // 鼠标按下时窗口左上角的坐标
protected:
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
private slots:
    void on_btn_close_clicked();
    void on_btn_video_clicked();
    void on_btn_cslive_clicked();
    void on_btn_liveroom_clicked();
    void on_btn_multiroom_clicked();
    void on_btn_custom_clicked();
private:
    Ui::MainDialog ui;

    CSLiveSetting* m_csLiveSetting;
    LiveRoomList* m_liveRoomList;
    RTCRoomList* m_RTCRoomList;
};
