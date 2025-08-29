#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QStack>
#include <QHash>

class QListWidgetItem;
class QListWidget;


#include <QTableWidget>
#include <QTableWidgetItem>
class QNetworkAccessManager;
class QNetworkReply;
#include <QMediaPlayer>
class QAudioOutput;
class QPixmap;
class QMenu;

class SongInfo;
class OnlineRequest;
class LocalSql;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TableWidget : public QTableWidget
{
    Q_OBJECT
public:
    // 获取 item 信息
    static QSharedPointer<SongInfo> getItemInfo(QTableWidgetItem*);

    // 初始化列数
    explicit TableWidget(QWidget *parent = nullptr);

    // 添加 row
    void append(QSharedPointer<SongInfo>);
    // 插入 row
    void insert(int index, QSharedPointer<SongInfo>);
    // 删除匹配 URL 的行
    void removeRow(QString url);
    // 查找 row
    int findRow(QString url);
};

class LyricWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 加载数据库配置
    void loadMysqlIni();
    // 加载播放器
    void loadMediaPlayer();
    // 设置 widget 样式
    void setStyleSheet();
    // 设置透明度
    void setOpacity(QWidget* widget, qreal opacity);
    // 设置信号槽
    void setEventHandler();
    // 设置窗口信息
    void initStackWindow();
    // 格式化排整每个页面
    void formatWidget(QWidget*, QString title = "", TableWidget* = nullptr);
    void on_itemClicked(QListWidgetItem *item);
    void changeWidget(int new_index);
    // 设置图标
    void setIcon(QTableWidgetItem* item, QByteArray icon_data);
protected:
    void closeEvent(QCloseEvent*) override;
    // 将背景图片四周裁剪为圆角
    void paintEvent(QPaintEvent *event) override;

    // 拖动窗口
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    // 最大化窗口
    void showMaxOrNormal();

private slots:
    // 响应 table 中双击信号==>更改播放列表内容
    void on_tableItemDoubleClicked(QTableWidgetItem* item);
    // 解析网络相回复的 json 数据，并插入到 search_widget 中
    void decodeFromJson(QWidget* search_widget, QByteArray response);
    // 下载 item 歌曲对应的图标
    void downloadItemIcon(QTableWidgetItem *item, const QString iconUrl);
    // 播放音乐，默认由 loopStatus 按钮控制播放顺序
    void playMedia(int playIndex = -1);
    // 同步播放显示
    void syncPlay(QString source);
    // 播放结束
    void on_playingStopped(bool playing);
    // 播放出错
    void on_playingError(QMediaPlayer::Error error, const QString &errorString);
    // 界面上的 MUSIC 按钮响应 ==> 回到主界面
    void on_m_btnMain_clicked();
    // 界面上的返回按钮 ==> 回到上一个界面
    void on_m_btnBackWidget_clicked();
    // 搜索按钮点击后的处理
    void on_m_btnSearch_clicked();
    // TODO： 个人登录信息 ==> 对应服务端还没有做
    void on_m_btnInfo_clicked();
    // TODO: about 信息
    void on_m_btnNotify_clicked();
    // TODO: setting 设置字体，界面，音频设备等
    void on_m_btnSetting_clicked();
    // 换肤
    void on_m_btnSkin_clicked();
    // TODO：迷你模式
    void on_m_btnMini_clicked();
    // 点击喜欢按钮
    void on_m_btnMediaLove_clicked();
    // 播放上一首
    void on_m_btnGoPrevious_clicked();
    // 暂停/开启 播放
    void on_m_btnGoPlay_clicked();
    // 播放下一首
    void on_m_btnGoNext_clicked();
    // TODO: 循环播放
    void on_m_btnMediaLoop_clicked();
    // 点击右下方词->显示歌词
    void on_m_btnMediaLyric_clicked();
    // TODO: 设置音量
    void on_m_btnMediaSound_clicked();
    // 播放进度条滑动
    void on_m_sliderMediaTicker_sliderMoved(int position);
    // 点击左下方删除->从当前播放列表中移除正在播放的音乐
    void on_m_btnMediaDelete_clicked();
    // 点击左下方歌名->显示歌词
    void on_m_btnMediaName_clicked();
    // 点击左下方歌手->搜索歌手
    void on_m_btnMediaPlayer_clicked();

private:
    // 皮肤设置
    void backtoDefault();   // 默认皮肤
    void backtoPink();      // 粉色皮肤
    void backtoCustom();    // 自定义皮肤
    // 系统托盘
    void on_systemTray(QSystemTrayIcon::ActivationReason reason);
    void initSystemTray();
    void quitSystemTray();

    // 获取当前值
    QSharedPointer<SongInfo> getCurrentSong();

private:
    Ui::MainWindow *ui;
    // 页面历史记录
    QStack<int> m_history;
    QList<TableWidget*> m_tables;
    TableWidget* m_playlistTable;
    // 记录搜索页面，不用的及时清理
    QHash<int, QWidget*> m_searchHistory;
    // 页面对应的 table 控件
    QHash<QWidget*, TableWidget*> m_searchRes;

    QPixmap m_background;
    // 窗口拖动
    QRect m_dragValidRect;
    QPoint m_pressedPos; // 拖动时的相对位置
    bool m_isDrag{false};
    // 窗口缩放
    enum Edge { None = 0, Top = 1, Bottom = 2, Left = 4, Right = 8,
                TopRight = Top|Right,
                BottomRight = Bottom|Right };
    bool m_isResizing{false};
    Edge m_resizeEdge{None};
    QPoint m_dragStartPosition;
    QRect m_dragStartGeometry;
    int m_borderWidth{5};

    // 窗口最大化状态
    bool m_isMaximized{false};

    Edge getResizeEdge(const QPoint &pos);

    // 换肤
    QMenu* m_menuChangeSkin;
    // 系统托盘
    QSystemTrayIcon* m_systemTray;

    // 媒体
    QMediaPlayer* m_player;
    QAudioOutput* m_audio;
    QString m_playUrl;
    // 记录播放列表及时更新
    int m_currentPlayingIndex;
    QList<QString> m_playingHistory;
    // 记录当前播放循环状态
    int m_loopStatus;

    // 网络
    OnlineRequest* m_access;
    // 数据库
    LocalSql* m_sql;

    // 歌词窗口
    LyricWidget* m_lyricWidget;

    // 音量
    QSlider* m_volumeSlider;
    QTimer* m_volumeTimer;
};
#endif // MAINWINDOW_H
