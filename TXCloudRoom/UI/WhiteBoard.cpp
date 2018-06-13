#include "WhiteBoard.h"
#include <QWindow>
#include <QFileDialog>
#include <QTimer>
#include <ctime>
#include "QDialogProgress.h"
#include "BoardService.h"
#include "DataReport.h"
#include "log.h"

WhiteBoard::WhiteBoard(bool enableDraw, QWidget *parent)
	: QMainWindow(parent)
	, _enableDraw(enableDraw)
{
	emit QDialogProgress::instance().showProgress(QStringLiteral("正在同步数据"));

	setWindowFlags(windowFlags() & ~Qt::Window);

	ui.setupUi(this);
	ui.menuBar->hide();
	ui.mainToolBar->hide();
	ui.statusBar->hide();

	for(auto subWin : ui.centralWidget->subWindowList())
	{
		if(subWin->windowTitle() == "ToolsWindow")
		{
			_toolsWindow = subWin;
			_toolsWindow->setWindowFlags(Qt::FramelessWindowHint);
			_toolsWindow->setAttribute(Qt::WA_TranslucentBackground);
			_toolsWindow->setParent(this);
			_toolsWindow->hide();
		}
		else if(subWin->windowTitle() == "PagesWindow")
		{
			_pagesWindow = subWin;
			_pagesWindow->setWindowFlags(Qt::FramelessWindowHint);
			_pagesWindow->setAttribute(Qt::WA_TranslucentBackground);
			_pagesWindow->setParent(this);
			_pagesWindow->hide();
		}
		else if(subWin->windowTitle() == "ColorWindow")
		{
			_colorWindow = subWin;
			_colorWindow->setWindowFlags(Qt::FramelessWindowHint);
			_colorWindow->setAttribute(Qt::WA_TranslucentBackground);
			_colorWindow->hide();
		}
		else if(subWin->windowTitle() == "SizeWindow")
		{
			_sizeWindow = subWin;
			_sizeWindow->setWindowFlags(Qt::FramelessWindowHint);
			_sizeWindow->setAttribute(Qt::WA_TranslucentBackground);
			_sizeWindow->hide();
		}
		else if(subWin->windowTitle() == "ShapeWindow")
		{
			_shapeWindow = subWin;
			_shapeWindow->setWindowFlags(Qt::FramelessWindowHint);
			_shapeWindow->setAttribute(Qt::WA_TranslucentBackground);
			_shapeWindow->hide();
		}
		else if(subWin->windowTitle() == "DialogWindow")
		{
			_dialogWindow = subWin;
			_dialogWindow->setWindowFlags(Qt::FramelessWindowHint);
			_dialogWindow->setAttribute(Qt::WA_TranslucentBackground);
			_dialogWindow->setParent(this);
			_dialogWindow->hide();
		}
	}

	_timer = new QTimer(this);

	connectSignals();

	BoardService::instance().setCallback(this);
	BoardService::instance().syncEventData();

	_boardWindow = ui.centralWidget->addSubWindow(QWidget::createWindowContainer(QWindow::fromWinId((WId)BoardService::instance().getRenderWindow())));
	_boardWindow->setWindowTitle(QStringLiteral("BoardWindow"));
	_boardWindow->setWindowFlags(Qt::FramelessWindowHint);
	_boardWindow->show();

	ui.centralWidget->setActiveSubWindow(_toolsWindow);
	ui.centralWidget->setActiveSubWindow(_pagesWindow);
	ui.centralWidget->setActiveSubWindow(_colorWindow);
	ui.centralWidget->setActiveSubWindow(_sizeWindow);
	ui.centralWidget->setActiveSubWindow(_shapeWindow);
	ui.centralWidget->setActiveSubWindow(_dialogWindow);
	
	_colorStyle =
		R"(
QCheckBox{
	margin: 6px 4px 6px 4px;
}

QCheckBox:unchecked{
	border-image: url(:/tools/tools/pick-normal.png);
}

QCheckBox:checked{
	border-radius: 2px;
	background-color:#33000000;
	border-image: url(:/tools/tools/pick-press.png);
}

QCheckBox::indicator {
	background-color: %1;
	margin:5px;
    width: 18px;
    height: 14px;
	image: none;
}
)";

	updateTool();

	on_updatePageWindow();

	_timer->start(100);
}

WhiteBoard::~WhiteBoard()
{
	_timer->stop();
}

void WhiteBoard::setEnableDraw(bool enableDraw)
{
	_enableDraw = enableDraw;
	if(_enableDraw)
	{
		showToolsWindow();
		showPagesWindow();
	}
	else
	{
		hideToolsWindow();
		hidePagesWindow();
		BoardService::instance().useTool(BoardTool::None);
	}
}

void WhiteBoard::setMainUser(bool mainUser)
{
	_mainUser = mainUser;
	if (_enableDraw)
	{
		showPagesWindow();
	}
}

void WhiteBoard::resizeEvent(QResizeEvent * event)
{
	_toolsWindow->resize(36, 288);
	_toolsWindow->move(0, (ui.centralWidget->height() - _toolsWindow->height()) / 2);

	_pagesWindow->resize(ui.centralWidget->width(), 42);
	_pagesWindow->move(0, ui.centralWidget->height() - _pagesWindow->height());

	_boardWindow->resize(ui.centralWidget->width(), ui.centralWidget->height() - _pagesWindow->height());
	_boardWindow->move(0, 0);
}

void WhiteBoard::showEvent(QShowEvent* event)
{
	setEnableDraw(_enableDraw);
}

bool WhiteBoard::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == _toolsWindow)
	{
		if (event->type() == QEvent::Type::Enter)
		{
			showToolsWindow();
		}
	}
	else if(watched == _pagesWindow)
	{
		if (event->type() == QEvent::Type::Enter)
		{
			showPagesWindow();
		}
	}
	return QMainWindow::eventFilter(watched, event);
}

void WhiteBoard::on_chkColorPick_toggled(bool checked)
{
	if(checked)
	{
		showColorWindow();
	}
	else
	{
		hideColorWindow();
	}
}

void WhiteBoard::on_btnClear_clicked()
{
	BoardService::instance().clear();
}

void WhiteBoard::on_btnUpload_clicked()
{
	DataReport::instance().setClickUpload(DataReport::instance().txf_gettickcount());

	QString fileName = QFileDialog::getOpenFileName(this,
		QStringLiteral("选择背景文件"),
		"",
		QStringLiteral("背景文件 (*.png *.jpg *.jpeg *.bmp *.ppt *.pptx *.pdf *.doc)"),
		nullptr);
	if (fileName.isEmpty()) return;
	emit QDialogProgress::instance().showProgress(QStringLiteral("正在上传"), 0);
	
	QFileInfo finfo(fileName);
	DataReport::instance().setFileSize(finfo.size());

	BoardService::instance().uploadFile(fileName.toStdWString());
}

void WhiteBoard::on_tool_toggled(bool checked)
{
	if(checked)
	{
		updateTool(true);
	}
}

void WhiteBoard::on_color_toggled(bool checked)
{
	if(checked)
	{
		updateColor();
		hideColorWindow();
	}
}

void WhiteBoard::on_size_toggled(bool checked)
{
	if(checked)
	{
		updateSize();
		hideSizeWindow();
	}
}

void WhiteBoard::on_shape_toggled(bool checked)
{
	if(checked)
	{
		updateShape();
		hideShapeWindow();
	}
}

void WhiteBoard::on_btnUndo_clicked()
{
	BoardService::instance().undo();
}

void WhiteBoard::on_btnRedo_clicked()
{
	BoardService::instance().redo();
}

void WhiteBoard::on_btnLastPage_clicked()
{
	BoardService::instance().gotoLastPage();
	on_updatePageWindow();
}

void WhiteBoard::on_btnPageNumber_clicked()
{
}

void WhiteBoard::on_btnNextPage_clicked()
{
	BoardService::instance().gotoNextPage();
	on_updatePageWindow();
}

void WhiteBoard::on_btnAddPage_clicked()
{
	BoardService::instance().insertPage();
	on_updatePageWindow();
}

void WhiteBoard::on_btnDeletePage_clicked()
{
	showDialogWindow();
}

void WhiteBoard::on_sliderShapeThin_valueChanged(int value)
{
	ui.labelThinValue->setText(QString::number(value));
	updateShape();
}

void WhiteBoard::on_updateWindowStatus()
{
	const clock_t curClock = clock();

	if(_toolsWindow->underMouse())
	{
		_toolsLostClock = curClock;
	}
	if(_pagesWindow->underMouse())
	{
		_pagesLostClock = curClock;
	}
	if(_colorWindow->underMouse())
	{
		_colorLostClock = curClock;
	}
	if(_sizeWindow->underMouse())
	{
		_sizeLostClock = curClock;
	}
	if(_shapeWindow->underMouse())
	{
		_shapeLostClock = curClock;
	}

	if(_toolsLostClock != 0 && curClock - _toolsLostClock > 1000)
	{
		//hideToolsWindow();
		_toolsLostClock = 0;
	}
	if(_pagesLostClock != 0 && curClock - _pagesLostClock > 1000)
	{
		//hidePagesWindow();
		_pagesLostClock = 0;
	}
	if(_colorLostClock != 0 && curClock - _colorLostClock > 1000)
	{
		hideColorWindow();
		_colorLostClock = 0;
	}
	if(_sizeLostClock != 0 && curClock - _sizeLostClock > 1000)
	{
		hideSizeWindow();
		_sizeLostClock = 0;
	}
	if(_shapeLostClock != 0 && curClock - _shapeLostClock > 1000)
	{
		hideShapeWindow();
		_shapeLostClock = 0;
	}
}

void WhiteBoard::on_updatePageWindow()
{
	ui.btnPageNumber->setText(
		QString::number(BoardService::instance().getPageIndex() + 1) +
		" / " +
		QString::number(BoardService::instance().getPageCount()));
	ui.btnLastPage->setEnabled(BoardService::instance().getPageIndex() > 0);
	ui.btnNextPage->setEnabled(BoardService::instance().getPageIndex() < BoardService::instance().getPageCount() - 1);
	ui.btnDeletePage->setEnabled(BoardService::instance().getPageCount() > 1);
}

void WhiteBoard::on_btnDialogClose_clicked()
{
	hideDialogWindow();
}

void WhiteBoard::on_btnDialogOK_clicked()
{
	BoardService::instance().deletePage();
	on_updatePageWindow();
	hideDialogWindow();
}

void WhiteBoard::on_btnDialogCancel_clicked()
{
	hideDialogWindow();
}

void WhiteBoard::connectSignals()
{
	connect(this, &WhiteBoard::updatePageWindow, this, &WhiteBoard::on_updatePageWindow);
	connect(this, &WhiteBoard::setUndoEnabled, ui.btnUndo, &QWidget::setEnabled);
	connect(this, &WhiteBoard::setRedoEnabled, ui.btnRedo, &QWidget::setEnabled);
	//connect(this, &WhiteBoard::setCopyEnabled, ui.btnCopy, &QWidget::setEnabled);
	//connect(this, &WhiteBoard::setRemoveEnabled, ui.btnRemove, &QWidget::setEnabled);

	connect(ui.rdoPen, &QRadioButton::clicked, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoPen, &QRadioButton::toggled, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoLine, &QRadioButton::clicked, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoLine, &QRadioButton::toggled, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoEraser, &QRadioButton::clicked, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoEraser, &QRadioButton::toggled, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoSelect, &QRadioButton::clicked, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoSelect, &QRadioButton::toggled, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoShape, &QRadioButton::clicked, this, &WhiteBoard::on_tool_toggled);
	connect(ui.rdoShape, &QRadioButton::toggled, this, &WhiteBoard::on_tool_toggled);

	connect(ui.rdoColorBlue, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorBlue, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorGreen, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorGreen, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorOrange, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorOrange, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorRed, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorRed, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorBlack, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorBlack, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorGrey, &QRadioButton::clicked, this, &WhiteBoard::on_color_toggled);
	connect(ui.rdoColorGrey, &QRadioButton::toggled, this, &WhiteBoard::on_color_toggled);

	connect(ui.rdoSizeLevel1, &QRadioButton::clicked, this, &WhiteBoard::on_size_toggled);
	connect(ui.rdoSizeLevel1, &QRadioButton::toggled, this, &WhiteBoard::on_size_toggled);
	connect(ui.rdoSizeLevel2, &QRadioButton::clicked, this, &WhiteBoard::on_size_toggled);
	connect(ui.rdoSizeLevel2, &QRadioButton::toggled, this, &WhiteBoard::on_size_toggled);
	connect(ui.rdoSizeLevel3, &QRadioButton::clicked, this, &WhiteBoard::on_size_toggled);
	connect(ui.rdoSizeLevel3, &QRadioButton::toggled, this, &WhiteBoard::on_size_toggled);

	connect(ui.rdoShapeEmptyRect, &QRadioButton::clicked, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeEmptyRect, &QRadioButton::toggled, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeFullRect, &QRadioButton::clicked, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeFullRect, &QRadioButton::toggled, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeEmptyEllipse, &QRadioButton::clicked, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeEmptyEllipse, &QRadioButton::toggled, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeFullEllipse, &QRadioButton::clicked, this, &WhiteBoard::on_shape_toggled);
	connect(ui.rdoShapeFullEllipse, &QRadioButton::toggled, this, &WhiteBoard::on_shape_toggled);

	connect(_timer, &QTimer::timeout, this, &WhiteBoard::on_updateWindowStatus);
}

void WhiteBoard::updateTool(bool showToolWindow)
{
	if(ui.rdoPen->isChecked())
	{
		updateColor();
		updateSize();
		BoardService::instance().useTool(BoardTool::Pen);
		if(showToolWindow)
		{
			showSizeWindow();
		}
	}
	else if(ui.rdoLine->isChecked())
	{
		updateColor();
		updateSize();
		BoardService::instance().useTool(BoardTool::Line);
		if(showToolWindow)
		{
			showSizeWindow();
		}
	}
	else if (ui.rdoEraser->isChecked())
	{
		updateColor();
		updateSize();
		BoardService::instance().useTool(BoardTool::Eraser);
		//if (showToolWindow)
		//{
		//	showSizeWindow();
		//}
	}
	else if(ui.rdoSelect->isChecked())
	{
		BoardService::instance().useTool(BoardTool::Select);
	}
	else if(ui.rdoShape->isChecked())
	{
		updateColor();
		updateShape();
		if(showToolWindow)
		{
			showShapeWindow();
		}
	}
}

void WhiteBoard::updateColor()
{
	if(ui.rdoColorBlue->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #0075ff");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0x0075ffff);
	}
	else if(ui.rdoColorGreen->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #00cc00");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0x00cc00ff);
	}
	else if(ui.rdoColorOrange->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #ff9903");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0xff9903ff);
	}
	else if(ui.rdoColorRed->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #ff0100");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0xff0100ff);
	}
	else if(ui.rdoColorBlack->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #000000");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0x000000ff);
	}
	else if(ui.rdoColorGrey->isChecked())
	{
		QString updateStyle = QString(_colorStyle).arg(" #cccccc");
		ui.chkColorPick->setStyleSheet(updateStyle);
		BoardService::instance().setColor(0xccccccff);
	}
}

void WhiteBoard::updateSize()
{
	if(ui.rdoSizeLevel1->isChecked())
	{
		BoardService::instance().setWidth(50);
	}
	else if(ui.rdoSizeLevel2->isChecked())
	{
		BoardService::instance().setWidth(100);
	}
	else if(ui.rdoSizeLevel3->isChecked())
	{
		BoardService::instance().setWidth(150);
	}
}

void WhiteBoard::updateShape()
{
	if (ui.rdoShapeEmptyRect->isChecked())
	{
		BoardService::instance().useTool(BoardTool::Rectangle);
		BoardService::instance().setFill(false);
	}
	else if (ui.rdoShapeFullRect->isChecked())
	{
		BoardService::instance().useTool(BoardTool::Rectangle);
		BoardService::instance().setFill(true);
	}
	else if (ui.rdoShapeEmptyEllipse->isChecked())
	{
		BoardService::instance().useTool(BoardTool::Ellipse);
		BoardService::instance().setFill(false);
	}
	else if (ui.rdoShapeFullEllipse->isChecked())
	{
		BoardService::instance().useTool(BoardTool::Ellipse);
		BoardService::instance().setFill(true);
	}

	BoardService::instance().setWidth(ui.sliderShapeThin->value() * 50);
}

void WhiteBoard::showToolsWindow()
{
	_toolsWindow->hide();
	_toolsWindow->resize(36, 288);
	_toolsWindow->move(0, (ui.centralWidget->height() - _toolsWindow->height()) / 2);
	_toolsWindow->show();
	
	_toolsLostClock = clock();
}

void WhiteBoard::hideToolsWindow()
{
	_toolsWindow->hide();
}

void WhiteBoard::showPagesWindow()
{
	if (_mainUser)
	{
		_pagesWindow->hide();
		_pagesWindow->resize(ui.centralWidget->width(), 42);
		_pagesWindow->move(0, ui.centralWidget->height() - _pagesWindow->height());
		_pagesWindow->show();

		_pagesLostClock = clock();
	}
}

void WhiteBoard::hidePagesWindow()
{
	_pagesWindow->hide();
}

void WhiteBoard::showColorWindow()
{
	_colorWindow->move(36, _toolsWindow->y() + 0);
	_colorWindow->show();
	_colorLostClock = clock();
}

void WhiteBoard::hideColorWindow() const
{
	_colorWindow->hide();
	ui.chkColorPick->setChecked(false);
}

void WhiteBoard::showSizeWindow()
{
	if (ui.rdoPen->isChecked()){
		_sizeWindow->move(36, _toolsWindow->y() + 36);
	}
	else if (ui.rdoLine->isChecked()){
		_sizeWindow->move(36, _toolsWindow->y() + 36 * 2);
	}
	else if (ui.rdoEraser->isChecked()) {
		_sizeWindow->move(36, _toolsWindow->y() + 36 * 4);
	}
	_sizeWindow->show();
	_sizeLostClock = clock();
}

void WhiteBoard::hideSizeWindow() const
{
	_sizeWindow->hide();
}

void WhiteBoard::showShapeWindow()
{
	_shapeWindow->move(36, _toolsWindow->y() + 36 * 3);
	_shapeWindow->show();
	_shapeLostClock = clock();
}

void WhiteBoard::hideShapeWindow() const
{
	_shapeWindow->hide();
}

void WhiteBoard::showDialogWindow() const
{
	_dialogWindow->move(0, 0);
	_dialogWindow->resize(ui.centralWidget->size());
	_dialogWindow->show();
}

void WhiteBoard::hideDialogWindow() const
{
	_dialogWindow->hide();
}

void WhiteBoard::onUploadProgress(int percent)
{
	emit QDialogProgress::instance().showProgress(QStringLiteral("正在上传"), percent);
}

void WhiteBoard::onUploadResult(bool success)
{
	if(success)
	{
		emit QDialogProgress::instance().hideAfter(0);
		emit updatePageWindow();
	}
	else
	{
		emit QDialogProgress::instance().showProgress(QStringLiteral("上传失败"), 100);
		emit QDialogProgress::instance().hideAfter(1000);
	}
}

void WhiteBoard::onStatusChanged(bool canUndo, bool canRedo, bool canCopy, bool canRemove)
{
	emit setUndoEnabled(canUndo);
	emit setRedoEnabled(canRedo);
	//emit setCopyEnabled(canCopy);
	//emit setRemoveEnabled(canRemove);
}

void WhiteBoard::onSyncEventResult(bool success)
{
	emit QDialogProgress::instance().hideAfter(0);
}
