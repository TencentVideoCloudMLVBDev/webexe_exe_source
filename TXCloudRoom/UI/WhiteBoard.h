#pragma once

#include <QMainWindow>
#include <QMdiSubWindow>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <mutex>
#include <queue>
#include "ui_WhiteBoard.h"
#include "BoardService.h"

class WhiteBoard : public QMainWindow, public BoardServiceCallback
{
	Q_OBJECT

public:
	WhiteBoard(bool enableDraw, QWidget *parent = Q_NULLPTR);
	~WhiteBoard();

	void setEnableDraw(bool enableDraw);
	void setMainUser(bool mainUser);

protected:
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

signals:
	void updatePageWindow();
	void setUndoEnabled(bool enabled);
	void setRedoEnabled(bool enabled);

private slots:
	void on_chkColorPick_toggled(bool checked);
	void on_btnClear_clicked();
	void on_btnUpload_clicked();

	void on_tool_toggled(bool checked);
	void on_color_toggled(bool checked);
	void on_size_toggled(bool checked);
	void on_shape_toggled(bool checked);

	void on_btnUndo_clicked();
	void on_btnRedo_clicked();
	void on_btnLastPage_clicked();
	void on_btnPageNumber_clicked();
	void on_btnNextPage_clicked();
	void on_btnAddPage_clicked();
	void on_btnDeletePage_clicked();

	void on_sliderShapeThin_valueChanged(int value);

	void on_updateWindowStatus();

	void on_updatePageWindow();

	void on_btnDialogClose_clicked();
	void on_btnDialogOK_clicked();
	void on_btnDialogCancel_clicked();
private:
	void connectSignals();

	void updateTool(bool showToolWindow = false);
	void updateColor();
	void updateSize();
	void updateShape();

	void showToolsWindow();
	void hideToolsWindow();

	void showPagesWindow();
	void hidePagesWindow();

	void showColorWindow();
	void hideColorWindow() const;

	void showSizeWindow();
	void hideSizeWindow() const;

	void showShapeWindow();
	void hideShapeWindow() const;

	void showDialogWindow() const;
	void hideDialogWindow() const;

private:
	void onUploadProgress(int percent) override;
	void onUploadResult(bool success) override;
	void onStatusChanged(bool canUndo, bool canRedo) override;
	void onSyncEventResult(bool success) override;

private:
	Ui::WhiteBoard ui{};

	bool _enableDraw{false};

	clock_t _toolsLostClock{};
	clock_t _pagesLostClock{};
	clock_t _colorLostClock{};
	clock_t _sizeLostClock{};
	clock_t _shapeLostClock{};
	QMdiSubWindow *_boardWindow{};
	QMdiSubWindow *_toolsWindow{};
	QMdiSubWindow *_pagesWindow{};
	QMdiSubWindow *_colorWindow{};
	QMdiSubWindow *_sizeWindow{};
	QMdiSubWindow *_shapeWindow{};
	QMdiSubWindow *_dialogWindow{};

	bool _mainUser{false};
	QTimer *_timer{};
	QString _colorStyle;
};
