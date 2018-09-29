#include "PPTHistory.h"
#include "json.h"
#include "log.h"
#include "PPTHistoryItemView.h"
#include "BoardService.h"

#include <QtGui/QMouseEvent>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QScrollBar>
#include <Dwmapi.h>
#include <iostream>
#include <fstream>

PPTHistory::PPTHistory(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    connect(this, SIGNAL(tiggleRemoveItem(QString)), this, SLOT(on_removeItem(QString)), Qt::QueuedConnection);

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

    ui.listWidget->verticalScrollBar()->setStyleSheet(scrollBarStyle);

    load();
}

PPTHistory::~PPTHistory()
{

}

void PPTHistory::load()
{
    QString path = QCoreApplication::applicationDirPath() + "/ppt-history.json";

    LINFO(L"path: %s", path.toStdWString().c_str());

    std::filebuf fb;
    bool ret = fb.open(path.toStdWString(), std::ios::in);
    if (false == ret)
    {
        LTRACE(L"file open faled");
        return;
    }

    std::istream is(&fb);

    Json::Value root;
    Json::Reader reader;
    ret = reader.parse(is, root);
    if (false == ret)
    {
        fb.close();

        LTRACE(L"json parse faled");
        return;
    }

    Json::Value items;
    if (root.isMember("items"))
    {
        items = root["items"];
    }

    if (false == items.isArray())
    {
        fb.close();

        LTRACE(L"Not json array");
        return;
    }

    ui.listWidget->clear();
    for (int i = 0; i < items.size(); ++i)
    {
        Json::Value itemObj = items[i];

        std::string objName = itemObj["objName"].asString();
        std::string dateTime = itemObj["dateTime"].asString();

        on_addItem(objName.c_str(), dateTime.c_str());
    }

    fb.close();
}

void PPTHistory::save()
{
    Json::Value items;
    for (int i = 0; i < ui.listWidget->count(); ++i)
    {
        QListWidgetItem* item = ui.listWidget->item(i);
        PPTHistoryItemView* itemView = dynamic_cast<PPTHistoryItemView*>(ui.listWidget->itemWidget(item));
        if (nullptr == itemView)
        {
            continue;
        }

        Json::Value itemObj;
        itemObj["objName"] = itemView->objName().toStdString();
        itemObj["dateTime"] = itemView->dateTime().toStdString();
        items.append(itemObj);
    }

    Json::Value root;
    root["items"] = items;

    QString path = QCoreApplication::applicationDirPath() + "/ppt-history.json";

    LINFO(L"path: %s", path.toStdWString().c_str());

    std::filebuf fb;
    bool ret = fb.open(path.toStdWString(), std::ios::out);
    if (false == ret)
    {
        LERROR(L"file open faled");
        return;
    }

    std::ostream os(&fb);

    Json::StyledStreamWriter writer;
    writer.write(os, root);

    fb.close();
}

void PPTHistory::on_addItem(QString objName, QString dateTime)
{
    removeItem(objName);

    PPTHistoryItemView* itemView = new PPTHistoryItemView(this);
    itemView->setObjName(objName);
    itemView->setDateTime(dateTime);

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(itemView->size());
    ui.listWidget->insertItem(0, item);
    ui.listWidget->setItemWidget(item, itemView);
}

void PPTHistory::removeItem(const QString& objName)
{
    for (int i = 0; i < ui.listWidget->count(); ++i)
    {
        QListWidgetItem* item = ui.listWidget->item(i);
        PPTHistoryItemView* itemView = dynamic_cast<PPTHistoryItemView*>(ui.listWidget->itemWidget(item));
        if (itemView && itemView->isSameFile(objName))
        {
            item = ui.listWidget->takeItem(i);

            delete itemView;
            delete item;
        }
    }
}

void PPTHistory::on_removeItem(QString objName)
{
    removeItem(objName);
    save();
}

void PPTHistory::on_btn_close_clicked()
{
    save();

    hide();
}

void PPTHistory::on_btn_ok_clicked()
{
    QList<QListWidgetItem*> selectedItems = ui.listWidget->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i)
    {
        QListWidgetItem* item = selectedItems[i];
        PPTHistoryItemView* itemView = dynamic_cast<PPTHistoryItemView*>(ui.listWidget->itemWidget(item));
        if (nullptr == itemView)
        {
            continue;
        }

        BoardService::instance().openHistoryFile(itemView->objName().toStdWString());

        break;  // 仅打开第一个被选中项
    }

    save();

    hide();
}

void PPTHistory::on_btn_cancel_clicked()
{
    save();

    hide();
}

void PPTHistory::on_listWidget_itemDoubleClicked(QListWidgetItem * item)
{
    PPTHistoryItemView* itemView = dynamic_cast<PPTHistoryItemView*>(ui.listWidget->itemWidget(item));
    if (nullptr == itemView)
    {
        return;
    }

    BoardService::instance().openHistoryFile(itemView->objName().toStdWString());

    save();

    hide();
}

void PPTHistory::mousePressEvent(QMouseEvent *e)
{
    mousePressedPosition = e->globalPos();
    windowPositionAsDrag = pos();
}

void PPTHistory::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
        mousePressedPosition = QPoint();
}

void PPTHistory::mouseMoveEvent(QMouseEvent *e)
{
    if (!mousePressedPosition.isNull()) {
        QPoint delta = e->globalPos() - mousePressedPosition;
        move(windowPositionAsDrag + delta);
    }
}

void PPTHistory::showEvent(QShowEvent *)
{
	BOOL bEnable = false;
	::DwmIsCompositionEnabled(&bEnable);
	if (bEnable)
	{
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		::DwmSetWindowAttribute((HWND)winId(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		MARGINS margins = { -1 };
		::DwmExtendFrameIntoClientArea((HWND)winId(), &margins);
	}

    if (ui.listWidget->count() > 0)
    {
        QListWidgetItem* item = ui.listWidget->item(0);
        item->setSelected(true);
    }
}

