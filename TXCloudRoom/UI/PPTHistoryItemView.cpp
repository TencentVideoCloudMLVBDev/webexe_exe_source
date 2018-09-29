#include "PPTHistoryItemView.h"
#include "DialogMessage.h"

#include <assert.h>

PPTHistoryItemView::PPTHistoryItemView(PPTHistory* historyDlg, QWidget *parent)
    : QWidget(parent)
    , m_historyDlg(historyDlg)
    , m_objName("")
{
    assert(nullptr != m_historyDlg);

    m_ui.setupUi(this);
}

PPTHistoryItemView::~PPTHistoryItemView()
{

}

QString PPTHistoryItemView::objName() const
{
    return m_objName;
}

void PPTHistoryItemView::setObjName(const QString& name)
{
    m_objName = name;

    int index = name.indexOf('_');
    if (index + 1 < name.size())
    {
        QString fileName = name.mid(index + 1);
        m_ui.label_name->setText(fileName);
		m_ui.label_name->setToolTip(fileName);
        this->adjustSize();
    }
}

QString PPTHistoryItemView::fileName() const
{
    return m_ui.label_name->text();
}

void PPTHistoryItemView::setFileName(const QString& name)
{
    m_ui.label_name->setText(name);

    this->adjustSize();
}

QString PPTHistoryItemView::dateTime() const
{
    return m_ui.label_datetime->text();
}

void PPTHistoryItemView::setDateTime(const QString& dt)
{
    m_ui.label_datetime->setText(dt);

    this->adjustSize();
}

bool PPTHistoryItemView::isSameFile(const QString& name)
{
    QString selfFileName = m_objName;
    int index = m_objName.indexOf('_');
    if (index + 1 < m_objName.size())
    {
        selfFileName = m_objName.mid(index + 1);
    }

    QString otherFileName = name;
    index = name.indexOf('_');
    if (index + 1 < name.size())
    {
        otherFileName = name.mid(index + 1);
    }

    return (selfFileName == otherFileName);
}

void PPTHistoryItemView::on_btn_del_clicked()
{
    DialogMessage::ResultCode rc = DialogMessage::exec(QStringLiteral("ÊÇ·ñÈ·¶¨É¾³ý£¿"), DialogMessage::OK | DialogMessage::CANCEL);
    if (DialogMessage::Accepted == rc && m_historyDlg)
    {
        emit m_historyDlg->tiggleRemoveItem(m_objName);
    }
}
