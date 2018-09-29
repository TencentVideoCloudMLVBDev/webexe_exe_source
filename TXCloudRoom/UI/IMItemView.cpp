#include "IMItemView.h"

IMItemView::IMItemView(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
}

IMItemView::~IMItemView()
{
}

void IMItemView::setMsgContent(const QString& msg)
{
    m_ui.label_msg->setText(msg);
	m_ui.label_msg->setToolTip(msg);
    this->adjustSize();
}
