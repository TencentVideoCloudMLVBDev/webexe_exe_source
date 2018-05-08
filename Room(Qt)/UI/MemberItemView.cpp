#include "MemberItemView.h"

MemberItemView::MemberItemView(MemberPanel* panel, const MemberItem& members, QWidget *parent)
    : QWidget(parent)
    , m_panel(panel)
{
    m_ui.setupUi(this);

    m_ui.label_nick->setText(members.userName.c_str());
    m_ui.label_status->setText(members.status.c_str());
}

MemberItemView::~MemberItemView()
{

}
