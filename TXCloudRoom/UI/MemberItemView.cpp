#include "MemberItemView.h"

MemberItemView::MemberItemView(MemberPanel* panel, const MemberItem& members, QWidget *parent)
    : QWidget(parent)
    , m_panel(panel)
{
    m_ui.setupUi(this);
	if (members.role == MasterPusherRole)
	{
		m_ui.label_icon->setStyleSheet("image :url(:/RoomService/teacher.png)");

		QString labelStyle =
R"(
	background:  none;
	color: #0063ff;
	font: 10pt "Microsoft YaHei";
)";
		m_ui.label_nick->setStyleSheet(labelStyle);
	}
	else
	{
		m_ui.label_icon->setStyleSheet("image :url(:/RoomService/student.png)");
		QString labelStyle =
R"(
	background:  none;
	color: #333333;
	font: 10pt "Microsoft YaHei";
)";
		m_ui.label_nick->setStyleSheet(labelStyle);
	}

    m_ui.label_nick->setText(members.userName.c_str());

	if (!members.status.empty())
	{
		m_ui.label_status->setStyleSheet("image :url(:/RoomService/speaker.png)");
	}
    
}

MemberItemView::~MemberItemView()
{

}
