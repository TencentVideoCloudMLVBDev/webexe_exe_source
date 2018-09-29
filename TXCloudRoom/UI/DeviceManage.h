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
    void setDeviceManageEnable(bool enable);

	bool updateUI(int tabIndex, bool changeTabIndex = true);
	bool previewCamera();
	QWidget* getRenderWidget();

protected:
	virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

signals:
	void device_manage_ok(bool enableCamera, bool enableMic);
	void device_manage_cancel(int cameraIndex, int micIndex, int micVolume);
	void chb_camera_stateChanged(int state);
	void beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel);
	void record_manage_ok();
	void device_manage_tab_changed(int tabIndex);

private slots:
	void on_btn_close_clicked();
	void on_btn_ok_clicked();
	void on_btn_cancel_clicked();
	void on_chb_camera_stateChanged(int state);
	void on_btn_local_path_clicked();
	void on_editingFinished();
	void on_tabWidget_currentChanged();
	void on_radio_local_toggled();
	void on_radio_server_toggled();
	void on_radio_both_toggled();

private:
	void initBeautyManage();
	void initRecordManage();
	void initDeviceManage();

private:
	QPoint mousePressedPosition; 
	QPoint windowPositionAsDrag; 
	int m_micIndex;
	int m_cameraIndex;
	int m_micVolume;
	int m_tabIndex;
	bool m_bCameraEnable;
	bool m_bMicEnable;
	Ui::DeviceManage ui;
};
