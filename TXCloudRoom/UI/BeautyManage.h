#pragma once

#include <QDialog>
#include "ui_BeautyManage.h"

class BeautyManage : public QDialog
{
	Q_OBJECT

public:
	BeautyManage(QWidget *parent = Q_NULLPTR);
	~BeautyManage();
	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

signals:
	void beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel);

private slots:
	void on_btn_close_clicked();
	void on_btn_ok_clicked();
	void on_btn_cancel_clicked();

private:
	QPoint mousePressedPosition; 
	QPoint windowPositionAsDrag; 

	Ui::BeautyManage ui;
};
