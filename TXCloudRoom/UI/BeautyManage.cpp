#include "BeautyManage.h"
#include <QMouseEvent>
#include <QSettings>
#include <Dwmapi.h> 

BeautyManage::BeautyManage(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | windowFlags());

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

BeautyManage::~BeautyManage()
{
}

void BeautyManage::on_btn_ok_clicked()
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

	emit beauty_manage_ok(index, ui.slider_beauty->value(), ui.slider_white->value());
	hide();
}

void BeautyManage::on_btn_cancel_clicked()
{
	hide();
}

void BeautyManage::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void BeautyManage::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e)
		mousePressedPosition = QPoint();
}

void BeautyManage::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void BeautyManage::on_btn_close_clicked()
{
	hide();
}
