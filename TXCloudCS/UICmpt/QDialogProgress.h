#pragma once

#include <QDialog>
#include "ui_QDialogProgress.h"

class QDialogProgress : public QDialog
{
	Q_OBJECT

public:
	QDialogProgress(QWidget *parent = Q_NULLPTR);
	~QDialogProgress();

	static QDialogProgress& instance();

	void initInThread() const;

	void showEvent(QShowEvent* event) override;

	void hideEvent(QHideEvent* event) override;

signals:
	void showProgress(const QString& msg = "", int progress = -1);

	void hideAfter(int milliseconds = 1000);

private slots:
	void on_showProgress(const QString& msg, int progress);

	void on_hideAfter(int milliseconds = 1000);
private:
	Ui::QDialogProgress ui;
};
