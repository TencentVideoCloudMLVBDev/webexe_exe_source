
#ifndef TXShareFrameChoose_H
#define TXShareFrameChoose_H

#include <memory>
#include <QtCore/QRect>
#include <QtWidgets/QWidget>

class TXShareFrameChooseCB
{
public:
	virtual ~TXShareFrameChooseCB() {};
	virtual void chooseParam(HWND, QRect) = 0;
	virtual void cancelChoose() = 0;
};


class TXScreen;
class QMenu;
class TXShareFrameChoose : public QWidget {
    Q_OBJECT

signals:

    /**
     * @brief : 鼠标移动（信号）
     * @param : int x轴的坐标
     * @param : int y轴的坐标
     */
    void cursorPosChange(int, int);

    /**
     * @brief : 双击（信号）
     */
    void clickWnd(void);
public:
    /**
     * @brief : 构造函数
     * @note  : 当前依附的父窗口（一般不给父窗口）
     */
    explicit TXShareFrameChoose(QWidget *parent = 0);
    ~TXShareFrameChoose(void);

public:
	void start(TXShareFrameChooseCB *cb);
	void stop();
protected:

    /**
     * @brief : 隐藏窗口事件
     */
    virtual void hideEvent(QHideEvent *);
    /**
     * @brief : 关闭窗口事件
     */
    virtual void closeEvent(QCloseEvent *);
    /**
     * @brief : 双击事件
     */
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    /**
     * @brief : 鼠标按下事件
     */
    virtual void mousePressEvent(QMouseEvent *);
    /**
     * @brief : 鼠标释放事件
     */
    virtual void mouseReleaseEvent(QMouseEvent *e);
    /**
     * @brief : 鼠标移动事件
     */
    virtual void mouseMoveEvent(QMouseEvent *e);

    /**
     * @brief : 按键按下事件
     */
    virtual void keyPressEvent(QKeyEvent *e);
    /**
     * @brief : 自绘事件
     */
    virtual void paintEvent(QPaintEvent *);

    /**
     * @brief : 更新当前鼠标选区的窗口
     */
    void updateMouse(void);

private:
    /**
     * @brief : 初始化截屏背景
     * @return: QPixmap 经过暗色处理的屏幕图
     */
    std::shared_ptr<QPixmap> initGlobalScreen(void);


    /**
     * @brief : 初始化鼠标
     * @note  : 为鼠标设置默认状态下的图标样式
     * @param : ico 鼠标图片的资源文件路径
     * @remark: 若参数未填写，在使用本程序默认的鼠标Logo
     */
    void initCursor(const QString& ico = "");

    /**
     * @brief : 创建截图器
     * @note  : 若截图器已存在，则返回截图器示例，不会重复创建。
     * @param : pos 截图器的起始位置 （给当前鼠标位置即可）
     * @remark: 创建截图器前，需要创建相关的组件，(例：大小感知器，放大取色器)
     */
    std::shared_ptr<TXScreen> createScreen(const QPoint &pos);

    /**
     * @brief : 摧毁截图器
     * @note  : 若截图器已存在，则摧毁示例，并清理示例创建的连带资源
     * @date  : 2017年04月16日
     */
    void destroyScreen(void);


    /**
     * @brief : 获得当前屏幕的大小
     * @note  : 这个函数是支持多屏幕的，示例：双屏幕 QRect（-1920, 0, 3840, 1080）
     * @return: 返回 QRect 引用
     */
    const QRect& getScreenRect(void);

    /**
     * @brief : 获得屏幕的原画
     * @note  : 他不会重复获得屏幕原画，如果有，则返回原有的原画
     * @return: QPixmap* 指针
     * @date  : 2017年04月15日
     * @remark: 若想重新获得屏幕原画，需要清理原有资源
     */
    std::shared_ptr<QPixmap> getGlobalScreen(void);

private:

    /// 截屏窗口是否已经展示
    bool                        isLeftPressed_;
    /// 用于检测误操作
    QPoint                      startPoint_;
    /// 当前桌面屏幕的矩形数据
    QRect desktopRect_;
    /// 屏幕暗色背景图
    std::shared_ptr<QPixmap>    backgroundScreen_;
    /// 屏幕原画
    std::shared_ptr<QPixmap>    originPainting_;
    /// 截图屏幕
    std::shared_ptr<TXScreen>   screenTool_;

    /// 当前鼠标选区最小的矩形窗口
    QRect                       windowRect_;
    /// 置顶定时器
    /// 活动窗口
    static bool                 isActivity_;
	HWND					    m_curChooseHwnd;
	bool						m_bChooseWnd;

	TXShareFrameChooseCB		*m_pCB;
public slots:

    /**
     * @brief : Window下霸道置顶（唯我独尊）
     * @remark: 使用该函数时，会终止右键菜单的行为，慎重使用，避免BUG
     */
    void onEgoistic(void);

	/**
	* @brief : Window下霸道置顶（唯我独尊）
	* @remark: 使用该函数时，会终止右键菜单的行为，慎重使用，避免BUG
	*/
	void clickWndSlot(void);
};

/**
 * @class : TXScreen
 * @brief : 截图器
 * @note  : 主要关乎图片的编辑与保存
*/
class TXScreen : public QWidget {

    Q_OBJECT

signals:


    /**
     * @brief : 截图器大小修改（信号）
     * @param : int 宽度
     * @param : int 高度
     */
    void sizeChange(int,int);

    /**
     * @brief : 截图器窗口的位置（信号）
     * @param : int 窗口的横向位置
	 * @param : int 窗口的纵向位置
     */
    void postionChange(int,int);

    /**
     * @brief : 双击 （信号）
     */
    void doubleClick(void);

protected:

    /// 内边距，决定拖拽的触发。
    const int PADDING_ = 6;

    /// 方位枚举
    enum DIRECTION {
        UPPER=0,
        LOWER=1,
        LEFT,
        RIGHT,
        LEFTUPPER,
        LEFTLOWER,
        RIGHTLOWER,
        RIGHTUPPER,
        NONE
    };

public:

    explicit TXScreen(std::shared_ptr<QPixmap> originPainting, QPoint pos, QWidget *parent = 0);

	~TXScreen();

    /**
     * @brief : 获得当前截图器是否存在
     * @return: true : 存在
     */
    static bool state(void) { return isInit_; }
	QRect getWndQRect();
protected:

    /**
     * @brief : 呼出菜单事件
     */
    virtual void contextMenuEvent(QContextMenuEvent *);

    /**
     * @brief : 双击事件
     */
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    /**
     * @brief : 鼠标按下事件
     */
    virtual void mousePressEvent(QMouseEvent *e);

    /**
     * @brief : 鼠标释放事件
     */
    virtual void mouseReleaseEvent(QMouseEvent *e);
    /**
     * @brief : 鼠标移动事件
     */
    virtual void mouseMoveEvent(QMouseEvent *e);

    /**
     * @brief : 窗口移动事件
     */
    virtual void moveEvent(QMoveEvent *);

    /**
     * @brief : 窗口大小修改事件
     */
    virtual void resizeEvent(QResizeEvent *);


    /**
     * @brief : 窗口显示事件
     */
    virtual void showEvent(QShowEvent *);

    /**
     * @brief : 窗口隐藏事件
     */
    virtual void hideEvent(QHideEvent *);

    /**
     * @brief : 鼠标进入窗口事件
     */
    virtual void enterEvent(QEvent *e);

    /**
     * @brief : 鼠标离开窗口事件
     */
    virtual void leaveEvent(QEvent *e);

    /**
     * @brief : 窗口关闭事件
     */
    virtual void closeEvent(QCloseEvent *);

    /**
     * @brief : 界面自绘事件
     */
    virtual void paintEvent(QPaintEvent *);

public slots:


    /**
     * @brief : 根据鼠标位置修改窗口大小
     * @param : x 鼠标的横向位置
     * @param : y 鼠标的纵向位置
     */
    void onMouseChange(int x,int y);

    /**
     * @brief : 保存屏幕到剪切板中
     */
    void onChooseWndOrArea(void);

protected slots:
    /**
	 * @brief : 退出当前截图窗口
     */
    void quitScreenshot(void);

private:

    /// 是否已经设置初始大小
    static bool     isInit_;
    /// 窗口大小改变时，记录改变方向
    DIRECTION       direction_;
    /// 起点
    QPoint          originPoint_;
    /// 鼠标是否按下
    bool            isPressed_;
    /// 拖动的距离
    QPoint          movePos_;
    /// 标记锚点
    QPolygon        listMarker_;
    /// 屏幕原画
    std::shared_ptr<QPixmap> originPainting_;
	/// 当前窗口几何数据 用于绘制截图区域
	QRect			currentRect_;
    /// 右键菜单对象
    QMenu           *menu_;
};

#endif /// TXShareFrameChoose_H
