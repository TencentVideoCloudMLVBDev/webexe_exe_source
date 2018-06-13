#include "QDialogProgress.h"
#include <QTimer>
#include <QGraphicsDropShadowEffect>

QDialogProgress::QDialogProgress(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	setWindowFlags(Qt::Tool | Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
	shadowEffect->setOffset(0, 0);
	shadowEffect->setColor(Qt::gray);
	shadowEffect->setBlurRadius(5);
	ui.widgetMain->setGraphicsEffect(shadowEffect);

	ui.widgetProgress->setColor(Qt::white);

	initInThread();
}

QDialogProgress::~QDialogProgress()
{
}

QDialogProgress& QDialogProgress::instance()
{
	static QDialogProgress instance;
	return instance;
}

void QDialogProgress::initInThread() const
{
	connect(this, &QDialogProgress::showProgress, this, &QDialogProgress::on_showProgress);
	connect(this, &QDialogProgress::hideAfter, this, &QDialogProgress::on_hideAfter);
}

void QDialogProgress::showEvent(QShowEvent* event)
{
	ui.widgetProgress->startAnimation();
}

void QDialogProgress::hideEvent(QHideEvent* event)
{
	ui.widgetProgress->stopAnimation();
}

void QDialogProgress::on_showProgress(const QString& msg, int progress)
{
	ui.labelMsg->setText(msg);
	ui.labelMsg->setHidden(msg.isEmpty());
	ui.labelProgress->setText(QString::number(progress) + "%");
	ui.labelProgress->setVisible(progress >= 0 && progress <= 100);
	show();
}

void QDialogProgress::on_hideAfter(int milliseconds)
{
	if (milliseconds == 0)
	{
		hide();
	}
	else
	{
		QTimer::singleShot(milliseconds, [this]()
		{
			hide();
		});
	}
}
