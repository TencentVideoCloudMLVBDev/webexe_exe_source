#pragma once

#include <QWidget>
#include <QMainWindow>

#include "ui_MemberPanel.h"
#include "commonType.h"

typedef std::function<void(void)> txfunction;

class MemberPanel : public QWidget
{
    Q_OBJECT

public:
    MemberPanel(QWidget *parent = Q_NULLPTR);
    ~MemberPanel();

    void updateList(const std::list<MemberItem>& members);
protected slots:
    void onInviteBtnClicked();
private:
    Ui::MemberPanel ui;
};
