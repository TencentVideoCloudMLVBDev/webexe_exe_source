#include "DeviceManage.h"
#include <QMouseEvent>
#include <QSettings>
#include <Dwmapi.h> 
#include <QFileDialog>
#include "DialogMessage.h"
#include "log.h"

DeviceManage::DeviceManage(QWidget *parent)
	: QDialog(parent)
	, m_micIndex(0)
	, m_cameraIndex(0)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | windowFlags());

	initBeautyManage();
	initDeviceManage();
	initRecordManage();

	on_tabWidget_currentChanged();
}

DeviceManage::~DeviceManage()
{
}

void DeviceManage::setCameras(wchar_t ** camerasName, size_t capacity)
{
	ui.cmb_camera->clear();
	for (int i = 0; i < capacity; ++i)
	{
		ui.cmb_camera->addItem(QString::fromStdWString(camerasName[i]));
	}

	ui.cmb_camera->setCurrentIndex(0);
}

void DeviceManage::setMics(char ** micsName, size_t capacity)
{
	ui.cmb_mic->clear();
	for (int i = 0; i < capacity; ++i)
	{
		ui.cmb_mic->addItem(micsName[i]);
	}

	ui.cmb_mic->setCurrentIndex(0);
}

void DeviceManage::setMicVolume(int volume, bool init)
{
	ui.slider_volume->setValue(volume);
	if (init)
	{
		m_micVolume = volume;
	}
}

void DeviceManage::setDeviceManageEnable(bool enable)
{
    ui.tabWidget->setTabEnabled(2, enable);
}

bool DeviceManage::updateUI(int tabIndex, bool changeTabIndex)
{
	bool ret = false;

	if (changeTabIndex)
	{
		if (false == ui.tabWidget->isTabEnabled(2) && tabIndex == 2)
		{
			ui.tabWidget->setCurrentIndex(1);
		}
		else
		{
			ui.tabWidget->setCurrentIndex(tabIndex);
		}
	}

	switch (tabIndex)
	{
	case 0:
	{

	}
	break;
	case 1:
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/record-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		ui.spinBox->setValue(setting->value("sliceTime").toInt());
		bool local = setting->value("localRecord").toBool();
		bool server = setting->value("serverRecord").toBool();
		if (local && server)
		{
			ui.radio_both->setChecked(true);
		}
		else if (local)
		{
			ui.radio_local->setChecked(true);
		}
		else if (server)
		{
			ui.radio_server->setChecked(true);
		}
		ui.edit_local_path->setText(setting->value("localRecordPath").toString());
		ui.edit_server_url->setText(setting->value("serverUrl").toString());
		ui.edit_local_path->setCursorPosition(0);
		ui.edit_server_url->setCursorPosition(0);
		setting->endGroup();
	}
	break;
	case 2:
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		m_bCameraEnable = setting->value("enableCamera").toBool();
		m_bMicEnable = setting->value("enableMic").toBool();
		ui.chb_camera->setChecked(m_bCameraEnable);

		int cameraIndex = setting->value("cameraIndex").toInt();
		if (cameraIndex < ui.cmb_camera->count())
			ui.cmb_camera->setCurrentIndex(cameraIndex);
		else
			ui.cmb_camera->setCurrentIndex(0);

		ui.chb_mic->setChecked(m_bMicEnable);

		int micIndex = setting->value("micIndex").toInt();
		if (micIndex < ui.cmb_mic->count())
			ui.cmb_mic->setCurrentIndex(micIndex);
		else
			ui.cmb_mic->setCurrentIndex(0);

		setting->endGroup();
	}
	break;
	default:
		break;
	}

	ret = m_bCameraEnable;

	return ret;
}

bool DeviceManage::previewCamera()
{
	return ui.chb_camera->isChecked();
}

QWidget * DeviceManage::getRenderWidget()
{
	return ui.widget_render;
}

void DeviceManage::on_btn_ok_clicked()
{
	if (m_tabIndex == 0)
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/beauty-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		int index = 0;
		if (ui.radio_nature->isChecked())
		{
			index = 1;
		}
		else if (ui.radio_blur->isChecked())
		{
			index = 2;
		}
		setting->setValue("style", index);
		setting->setValue("beauty", ui.slider_beauty->value());
		setting->setValue("white", ui.slider_white->value());
		setting->endGroup();

		updateUI(2, false);
		emit beauty_manage_ok(index, ui.slider_beauty->value(), ui.slider_white->value());
		emit device_manage_ok(m_bCameraEnable, m_bMicEnable);
		hide();
	}
	else if (m_tabIndex == 1)
	{
		bool localRecord = false;
		bool serverRecord = false;
		if (ui.radio_local->isChecked())
		{
			localRecord = true;
		}
		else if (ui.radio_server->isChecked())
		{
			serverRecord = true;
		}
		else if (ui.radio_both->isChecked())
		{
			localRecord = true;
			serverRecord = true;
		}
	
		QString localPath = ui.edit_local_path->text();
		if (localRecord && localPath.isEmpty())
		{
			DialogMessage::exec(QStringLiteral("请输入本地录制保存路径!"), DialogMessage::OK);
			return;
		}

        QFileInfo file(localPath);
        if (localRecord && false == file.isDir())
        {
            DialogMessage::exec(QStringLiteral("请输入有效的文件夹路径!"), DialogMessage::OK);
            return;
        }

		QString serverUrl = ui.edit_server_url->text();
		if (serverRecord && serverUrl.isEmpty())
		{
			DialogMessage::exec(QStringLiteral("请输入服务器推流地址!"), DialogMessage::OK);
			return;
		}

        if (serverRecord && false == serverUrl.toLower().contains("rtmp://"))
        {
            DialogMessage::exec(QStringLiteral("请输入rtmp推流地址!"), DialogMessage::OK);
            return;
        }

		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/record-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("localRecord", localRecord);
		setting->setValue("localRecordPath", localPath);
		setting->setValue("sliceTime", ui.spinBox->value());
		setting->setValue("serverRecord", serverRecord);
		setting->setValue("serverUrl", serverUrl);
		setting->endGroup();

		updateUI(2, false);
		emit record_manage_ok();
		emit device_manage_ok(m_bCameraEnable, m_bMicEnable);
	}
	else if (m_tabIndex == 2)
	{
		bool enableCamera = ui.chb_camera->isChecked();
		int cameraIndex = ui.cmb_camera->currentIndex();
		bool enableMic = ui.chb_mic->isChecked();
		int micIndex = ui.cmb_mic->currentIndex();

		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("enableCamera", enableCamera);
		setting->setValue("cameraIndex", cameraIndex);
		setting->setValue("enableMic", enableMic);
		setting->setValue("micIndex", micIndex);
		setting->endGroup();

		m_bCameraEnable = enableCamera;
		m_bMicEnable = enableMic;
		emit device_manage_ok(enableCamera, enableMic);
	}
	hide();
}

void DeviceManage::on_btn_cancel_clicked()
{
	updateUI(2, false);
	emit device_manage_cancel(m_cameraIndex, m_micIndex, m_micVolume);
	hide();
}

void DeviceManage::on_chb_camera_stateChanged(int state)
{
	ui.cmb_camera->setEnabled(state == Qt::Checked);
	emit chb_camera_stateChanged(state);
}

void DeviceManage::on_btn_local_path_clicked()
{
	QString file_path = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择保存路径..."), "./");
	if (file_path.isEmpty())
	{
		return;
	}
	else
	{
		ui.edit_local_path->setText(file_path);
	}
}

void DeviceManage::on_editingFinished()
{
	ui.edit_local_path->setCursorPosition(0);
	ui.edit_server_url->setCursorPosition(0);
}

void DeviceManage::on_tabWidget_currentChanged()
{
	m_tabIndex = ui.tabWidget->currentIndex();

	switch (m_tabIndex)
	{
	case 0:
	{
		ui.tabWidget->setFixedHeight(270);
		this->setFixedHeight(400);
		this->update();
	}
	break;
	case 1:
	{
		ui.tabWidget->setFixedHeight(310);
		this->setFixedHeight(440);
		this->update();
	}
	break;
	case 2:
	{
		ui.tabWidget->setFixedHeight(500);
		this->setFixedHeight(636);
		this->update();
		emit device_manage_tab_changed(m_tabIndex);
	}
	break;
	}
}

void DeviceManage::on_radio_local_toggled()
{
	ui.label_record_tip->setText(QStringLiteral("本地文件分片时长范围为5~60分钟"));
	ui.label_slicetime->show();
	ui.label_path->show();
	ui.label_url->hide();
}

void DeviceManage::on_radio_server_toggled()
{
	ui.label_record_tip->setText("");
	ui.label_slicetime->hide();
	ui.label_path->hide();
	ui.label_url->show();
}

void DeviceManage::on_radio_both_toggled()
{
	ui.label_record_tip->setText(QStringLiteral("本地文件分片时长范围为5~60分钟"));
	ui.label_slicetime->show();
	ui.label_path->show();
	ui.label_url->show();
}

void DeviceManage::initBeautyManage()
{
	ui.slider_beauty->setMinimum(0);
	ui.slider_beauty->setMaximum(9);

	ui.slider_white->setMinimum(0);
	ui.slider_white->setMaximum(9);

	ui.radio_nature->setChecked(true);

	QFile file(QCoreApplication::applicationDirPath() + "/beauty-config.ini");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();

		ui.radio_nature->setChecked(true);
		ui.slider_beauty->setValue(5);
		ui.slider_white->setValue(5);
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/beauty-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("style", 1);
		setting->setValue("beauty", 5);
		setting->setValue("white", 5);
		setting->endGroup();
	}
	else
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/beauty-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		ui.slider_beauty->setValue(setting->value("beauty").toInt());
		ui.slider_white->setValue(setting->value("white").toInt());

		switch (setting->value("style").toInt())
		{
		case 0:
			ui.radio_smooth->setChecked(true);
			break;
		case 1:
			ui.radio_nature->setChecked(true);
			break;
		case 2:
			ui.radio_blur->setChecked(true);
			break;
		default:
			break;
		}

		setting->endGroup();
	}

	connect(this, SIGNAL(beauty_manage_ok(int, int, int)), this->parent(), SLOT(on_beauty_manage_ok(int, int, int)));
}

void DeviceManage::initRecordManage()
{
	ui.spinBox->setMinimum(5);
	ui.spinBox->setMaximum(120);
	ui.spinBox->setSingleStep(5);

	ui.label_slicetime->hide();
	ui.label_path->hide();
	ui.label_url->hide();

	QFile file(QCoreApplication::applicationDirPath() + "/record-config.ini");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();

		ui.spinBox->setValue(60);

		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/record-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("localRecord", true);
		setting->setValue("localRecordPath", "");
		setting->setValue("sliceTime", 60);
		setting->setValue("serverRecord", false);
		setting->setValue("serverUrl", "");
		setting->endGroup();
		ui.radio_local->setChecked(true);
	}
	else
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/record-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		bool local = setting->value("localRecord").toBool();
		bool server = setting->value("serverRecord").toBool();
		if (local && server)
		{
			ui.radio_both->setChecked(true);
		}
		else if (local)
		{
			ui.radio_local->setChecked(true);
		}
		else if (server)
		{
			ui.radio_server->setChecked(true);
		}
		
		ui.spinBox->setValue(setting->value("sliceTime").toInt());
		ui.edit_local_path->setText(setting->value("localRecordPath").toString());
		ui.edit_server_url->setText(setting->value("serverUrl").toString());
		setting->endGroup();
		ui.edit_local_path->setCursorPosition(0);
		ui.edit_server_url->setCursorPosition(0);
	}

	connect(ui.edit_local_path, SIGNAL(editingFinished()), this, SLOT(on_editingFinished()));
	connect(ui.edit_server_url, SIGNAL(editingFinished()), this, SLOT(on_editingFinished()));
	connect(this, SIGNAL(record_manage_ok()), this->parent(), SLOT(on_record_manage_ok()));
	connect(ui.radio_local, SIGNAL(clicked()), this, SLOT(on_radio_local_toggled()));
	connect(ui.radio_local, SIGNAL(toggled()), this, SLOT(on_radio_local_toggled()));
	connect(ui.radio_server, SIGNAL(clicked()), this, SLOT(on_radio_server_toggled()));
	connect(ui.radio_server, SIGNAL(toggled()), this, SLOT(on_radio_server_toggled()));
	connect(ui.radio_both, SIGNAL(clicked()), this, SLOT(on_radio_both_toggled()));
	connect(ui.radio_both, SIGNAL(toggled()), this, SLOT(on_radio_both_toggled()));
}

void DeviceManage::initDeviceManage()
{
	ui.slider_volume->setMinimum(0);
	ui.slider_volume->setMaximum(65535);
	ui.chb_camera->setChecked(true);
	ui.chb_mic->setChecked(true);
	m_bCameraEnable = true;
	m_bMicEnable = true;

	QFile file(QCoreApplication::applicationDirPath() + "/device-config.ini");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();
		ui.chb_camera->setChecked(true);
		ui.chb_mic->setChecked(true);
		ui.cmb_camera->setCurrentIndex(0);
		ui.cmb_mic->setCurrentIndex(0);

		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("enableCamera", true);
		setting->setValue("cameraIndex", 0);
		setting->setValue("enableMic", true);
		setting->setValue("micIndex", 0);
		setting->endGroup();
	}
	else
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("enableCamera", true);
		setting->setValue("enableMic", true);
		setting->endGroup();
		ui.edit_local_path->setCursorPosition(0);
		ui.edit_server_url->setCursorPosition(0);
	}

	connect(ui.cmb_mic, SIGNAL(currentIndexChanged(int)), this->parent(), SLOT(on_cmb_mic_currentIndexChanged(int)));
	connect(ui.cmb_camera, SIGNAL(currentIndexChanged(int)), this->parent(), SLOT(on_cmb_camera_currentIndexChanged(int)));
	connect(ui.slider_volume, SIGNAL(valueChanged(int)), this->parent(), SLOT(on_slider_volume_valueChanged(int)));
	connect(this, SIGNAL(device_manage_ok(bool, bool)), this->parent(), SLOT(on_device_manage_ok(bool, bool)));
	connect(this, SIGNAL(device_manage_cancel(int, int, int)), this->parent(), SLOT(on_device_manage_cancel(int, int, int)));
	connect(this, SIGNAL(chb_camera_stateChanged(int)), this->parent(), SLOT(on_chb_camera_stateChanged(int)));
}

void DeviceManage::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void DeviceManage::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e)
		mousePressedPosition = QPoint();
}

void DeviceManage::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void DeviceManage::on_btn_close_clicked()
{
	emit device_manage_cancel(m_cameraIndex, m_micIndex, m_micVolume);
	hide();
}

void DeviceManage::showEvent(QShowEvent * event)
{
	BOOL bEnable = false;
	::DwmIsCompositionEnabled(&bEnable);
	if (bEnable)
	{
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		::DwmSetWindowAttribute((HWND)winId(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		MARGINS margins = { -1 };
		::DwmExtendFrameIntoClientArea((HWND)winId(), &margins);
	}

	QDialog::showEvent(event);
}