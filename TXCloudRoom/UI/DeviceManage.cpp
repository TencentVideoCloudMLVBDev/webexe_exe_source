#include "DeviceManage.h"
#include <QMouseEvent>
#include <QSettings>
#include <Dwmapi.h> 

DeviceManage::DeviceManage(QWidget *parent)
	: QDialog(parent)
	, m_micIndex(0)
	, m_cameraIndex(0)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | windowFlags());

	ui.slider_volume->setMinimum(0);
	ui.slider_volume->setMaximum(65535);
	ui.chb_camera->setChecked(true);
	ui.chb_mic->setChecked(true);

	BOOL bEnable = false;
	::DwmIsCompositionEnabled(&bEnable);
	if (bEnable)
	{
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		::DwmSetWindowAttribute((HWND)winId(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		MARGINS margins = { -1 };
		::DwmExtendFrameIntoClientArea((HWND)winId(), &margins);
	}

	QFile file("device-config.ini");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();

		ui.chb_camera->setChecked(true);
		ui.chb_mic->setChecked(true);
		ui.cmb_camera->setCurrentIndex(0);
		ui.cmb_mic->setCurrentIndex(0);

		QSettings* setting = new QSettings("device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("enableCamera", true);
		setting->setValue("cameraIndex", 0);
		setting->setValue("enableMic", true);
		setting->setValue("micIndex", 0);
		setting->endGroup();
	}
	else
	{
		QSettings* setting = new QSettings("device-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("enableCamera", true);
		setting->setValue("enableMic", true);
		setting->endGroup();
	}

	connect(ui.cmb_mic, SIGNAL(currentIndexChanged(int)), this->parent(), SLOT(on_cmb_mic_currentIndexChanged(int)));
	connect(ui.cmb_camera, SIGNAL(currentIndexChanged(int)), this->parent(), SLOT(on_cmb_camera_currentIndexChanged(int)));
	connect(ui.slider_volume, SIGNAL(valueChanged(int)), this->parent(), SLOT(on_slider_volume_valueChanged(int)));
	connect(this, SIGNAL(device_manage_ok(bool, bool)), this->parent(), SLOT(on_device_manage_ok(bool, bool)));
	connect(this, SIGNAL(device_manage_cancel(int, int, int)), this->parent(), SLOT(on_device_manage_cancel(int, int, int)));
	connect(this, SIGNAL(chb_camera_stateChanged(int)), this->parent(), SLOT(on_chb_camera_stateChanged(int)));
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

bool DeviceManage::updateUI()
{
	QSettings* setting = new QSettings("device-config.ini", QSettings::IniFormat);
	setting->beginGroup("config");
	ui.chb_camera->setChecked(setting->value("enableCamera").toBool());

	int cameraIndex = setting->value("cameraIndex").toInt();
	if (cameraIndex < ui.cmb_camera->count())
		ui.cmb_camera->setCurrentIndex(cameraIndex);
	else
		ui.cmb_camera->setCurrentIndex(0);

	ui.chb_mic->setChecked(setting->value("enableMic").toBool());

	int micIndex = setting->value("micIndex").toInt();
	if (micIndex < ui.cmb_mic->count())
		ui.cmb_mic->setCurrentIndex(micIndex);
	else
		ui.cmb_mic->setCurrentIndex(0);

	setting->endGroup();

	return ui.chb_camera->isChecked();
}

QWidget * DeviceManage::getRenderWidget()
{
	return ui.widget_render;
}

void DeviceManage::on_btn_ok_clicked()
{
	bool enableCamera = ui.chb_camera->isChecked();
	int cameraIndex = ui.cmb_camera->currentIndex();
	bool enableMic = ui.chb_mic->isChecked();
	int micIndex = ui.cmb_mic->currentIndex();

	QSettings* setting = new QSettings("device-config.ini", QSettings::IniFormat);
	setting->beginGroup("config");
	setting->setValue("enableCamera", enableCamera);
	setting->setValue("cameraIndex", cameraIndex);
	setting->setValue("enableMic", enableMic);
	setting->setValue("micIndex", micIndex);
	setting->endGroup();

	emit device_manage_ok(enableCamera, enableMic);
	hide();
}

void DeviceManage::on_btn_cancel_clicked()
{
	emit device_manage_cancel(m_cameraIndex, m_micIndex, m_micVolume);
	hide();
}

void DeviceManage::on_chb_camera_stateChanged(int state)
{
	ui.cmb_camera->setEnabled(state == Qt::Checked);
	emit chb_camera_stateChanged(state);
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
