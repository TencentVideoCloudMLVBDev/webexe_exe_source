#pragma once

#include <QWidget>
#include "ui_MemberItemView.h"
#include "MemberPanel.h"

class MemberItemView : public QWidget
{
    Q_OBJECT

public:
    MemberItemView(MemberPanel* panel, const MemberItem& members, QWidget *parent = Q_NULLPTR);
    ~MemberItemView();
private:
    Ui::MemberItemView m_ui;
    MemberPanel* m_panel;
};
