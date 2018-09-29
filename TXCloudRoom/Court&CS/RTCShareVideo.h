#pragma once

#include <QtWidgets/QDialog>
#include "ui_DialogShareVideo.h"
#include "RTCVideoWidget.h"
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QWidget>
#include "commonType.h"

struct RTCCameraWidget
{
	RTCVideoWidget* cameraWidget;
	bool idle;
	std::string userID;
};

class RTCShareVideo : public QDialog
{
    Q_OBJECT

public:
    RTCShareVideo(QWidget *parent = Q_NULLPTR);
    ~RTCShareVideo();
	//0 remove -1 self
	void setUserInfo(std::string userName, std::string userID, bool bAdd = true, bool bStart = false);
	void updateUI();
	void setCameraSize(int cameraSize);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

signals:
	void share_video_close();

protected slots:
    void on_btn_close_clicked();
	void on_btn_max_clicked();

private:
	void initCameraWidget();
	void setWidth(int width);

private:
    Ui::DialogShareVideo ui;

    bool m_pressed;
    QPoint m_point;
	QScrollArea *scrollArea_camera;
	QVBoxLayout * hCameraLayout;
	std::vector<RTCCameraWidget*> m_vCameraWidgets;
	int  m_cameraSize;
	RTCVideoWidget * selfWidget;
	int m_width = 150;
	QRect m_deskRect;
	bool m_bMax = false;
	int m_videoCount = 0;
};
