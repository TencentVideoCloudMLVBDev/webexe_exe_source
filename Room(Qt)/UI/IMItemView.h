#pragma once

#include <QWidget>
#include "ui_IMItemView.h"

class IMItemView : public QWidget
{
    Q_OBJECT

public:
    IMItemView(QWidget *parent = Q_NULLPTR);
    ~IMItemView();

    void setMsgContent(const QString& msg);
private:
    Ui::IMItemView m_ui;
};
