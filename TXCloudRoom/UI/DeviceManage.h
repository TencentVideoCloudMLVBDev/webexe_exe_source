#pragma once

#include <QDialog>
#include "ui_DeviceManage.h"

class DeviceManage : public QDialog
{
	Q_OBJECT

public:
	DeviceManage(QWidget *parent = Q_NULLPTR);
	~DeviceManage();
	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

	void setCameras(wchar_t **camerasName, size_t capacity);
	void setMics(char **micsName, size_t capacity);
	void setMicVolume(int volume, bool init = false);
	bool updateUI();
	QWidget* getRenderWidget();
signals:
	void device_manage_ok(bool enableCamera, bool enableMic);
	void device_manage_cancel(int cameraIndex, int micIndex, int micVolume);
	void chb_camera_stateChanged(int state);

private slots:
	void on_btn_close_clicked();
	void on_btn_ok_clicked();
	void on_btn_cancel_clicked();
	void on_chb_camera_stateChanged(int state);

private:
	QPoint mousePressedPosition; 
	QPoint windowPositionAsDrag; 
	int m_micIndex;
	int m_cameraIndex;
	int m_micVolume;

	Ui::DeviceManage ui;
};
