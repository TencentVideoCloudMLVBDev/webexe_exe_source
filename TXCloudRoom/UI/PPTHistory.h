#pragma once

#include <QtWidgets/QDialog>
#include "ui_PPTHistory.h"

class PPTHistory : public QDialog
{
    Q_OBJECT

public:
    PPTHistory(QWidget *parent = Q_NULLPTR);
    ~PPTHistory();

    void load();
    void save();

    void removeItem(const QString& objName);

signals:
    void tiggleRemoveItem(QString objName);

private slots:
    void on_btn_close_clicked();
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem * item);
	void on_addItem(QString objName, QString dateTime);
    void on_removeItem(QString objName);

protected:
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

    void showEvent(QShowEvent *) override;

private:
    QPoint mousePressedPosition;
    QPoint windowPositionAsDrag;

    Ui::PPTHistory ui;
};
