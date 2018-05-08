#include "MemberPanel.h"
#include "MemberItemView.h"
#include "InvitePanel.h"

#include <QScrollBar>

MemberPanel::MemberPanel(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.btn_invite, SIGNAL(clicked()), this, SLOT(onInviteBtnClicked()));

	ui.btn_invite->hide();

	const QString scrollBarStyle =
		R"(QScrollBar{
    background: transparent;
	 width: 7px;
}

QScrollBar::handle {
    background-color: #dbdbdb;
    border-radius: 3px;
}

QScrollBar::handle:hover {
    background-color: #dfdfdf;
}

QScrollBar::handle:pressed {
    background-color: #cccccc;
}

QScrollBar::add-line, QScrollBar::sub-line {
    background: transparent;
    height: 0px;
    width: 0px;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

QScrollBar::up-arrow, QScrollBar::down-arrow {
    background: transparent;
    height: 0px;
    width: 0px;
})";
	ui.lw_member->verticalScrollBar()->setStyleSheet(scrollBarStyle);
}

MemberPanel::~MemberPanel()
{

}

void MemberPanel::updateList(const std::list<MemberItem>& members)
{
	ui.lw_member->clear();

    for (std::list<MemberItem>::const_iterator it = members.begin(); members.end() != it; ++it)
    {
        MemberItemView *itemView = new MemberItemView(this, *it);
        QListWidgetItem *widgetItem = new QListWidgetItem();
        widgetItem->setSizeHint(itemView->size());
        ui.lw_member->addItem(widgetItem);
        ui.lw_member->setItemWidget(widgetItem, itemView);
    }
}

void MemberPanel::onInviteBtnClicked()
{
	//InvitePanel panel(m_pMainWidget);
	//panel.setRoomId(m_roomCreator.c_str());
	//panel.setJoinUrl(m_joinUrl.c_str());
	//panel.exec();
}
