#pragma once

#include <QtWidgets/QWidget>
#include "ui_PPTHistoryItemView.h"
#include "PPTHistory.h"

class PPTHistoryItemView : public QWidget
{
    Q_OBJECT

public:
    PPTHistoryItemView(PPTHistory* historyDlg, QWidget *parent = Q_NULLPTR);
    ~PPTHistoryItemView();

    QString objName() const;
    void setObjName(const QString& name);

    QString fileName() const;
    void setFileName(const QString& name);

    QString dateTime() const;
    void setDateTime(const QString& dt);

    bool isSameFile(const QString& name);
private slots:
    void on_btn_del_clicked();
private:
    Ui::PPTHistoryItemView m_ui;
    PPTHistory* m_historyDlg;
    QString m_objName;
};
