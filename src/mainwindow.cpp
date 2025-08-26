#include "../include/mainwindow.h"
#include "ui_mainwindow.h"
#include "../include/utils.h"
#include "../include/localsql.h"
#include "../include/onlinerequest.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QTableWidget>
#include <QHeaderView>

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QMediaPlayer>
#include <QAudioOutput>

#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSystemTrayIcon>

#include <QThread>
// 背景图片
QString BG_DEFAULT = ":/images/green.jpg";
QString BG_PINK = ":/images/pink-cat.jpg";

// 状态图标
QString ICON_TOPLAY             = ":/images/play.svg";
QString ICON_TOPAUSE            = ":/images/stop.svg";
QString ICON_SEARCH_PLACEHOLD   = ":/images/search.svg";
QString ICON_NORMAL  = ":/images/normal.svg";
QString ICON_MAX     = ":/images/max.svg";
QString ICON_LOVED   = ":/images/loved.svg";
QString ICON_TOLOVE  = ":/images/to-love.svg";

// 标识
enum IDENTIFY_COLUMN {ICON = 0, TITLE, AUTHOR, URL, PIC, LRC};

QSharedPointer<SongInfo> TableWidget::getItemInfo(QTableWidgetItem* item)
{
    QTableWidget* tableWidget = item->tableWidget();

    int row = item->row();
    auto url_item = tableWidget->item(row, URL);
    if (!url_item || url_item->text().isEmpty()) {
        qDebug() << "歌曲不存在";
        return nullptr;
    }
    QString url = url_item->text();

    QSharedPointer<SongInfo> info(new SongInfo);
    if (tableWidget->item(row, TITLE))
        info->title     = tableWidget->item(row, TITLE)->text();
    if (tableWidget->item(row, AUTHOR))
        info->author    = tableWidget->item(row, AUTHOR)->text();
    if (tableWidget->item(row, URL))
        info->url       = tableWidget->item(row, URL)->text();
    if(tableWidget->item(row, PIC))
        info->pic       = tableWidget->item(row, PIC)->text();
    if(tableWidget->item(row, LRC)) {
        info->lrc       = tableWidget->item(row, LRC)->text();
    }

    return info;
}

TableWidget::TableWidget(QWidget *parent) : QTableWidget(parent) {
    setColumnCount(6);
    setHorizontalHeaderLabels({tr("专辑"), tr("歌曲名称"), tr("歌手"), tr("url"), tr("pic"), tr("lrc")});
    horizontalHeader()->setVisible(true);
    setColumnWidth(ICON, 50);
    setColumnWidth(TITLE, 400);
    setColumnWidth(AUTHOR, 200);
    // 表中记录并隐藏 url 和 lyc，点击播放的时候会用到 url 和 歌词，这个时候
    // 因为数据库表 searchlist_tb 不可用，当搜索歌曲时，会清除掉该表已有内容
    setColumnHidden(URL, true);
    setColumnHidden(PIC, true);
    setColumnHidden(LRC, true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setShowGrid(false);
    setIconSize(QSize(50, 50));
}

void TableWidget::append(QSharedPointer<SongInfo> info)
{
    int row = rowCount();
    insertRow(row);
    setRowHeight(row, 50);
    setItem(row, TITLE, new QTableWidgetItem(info->title));
    setItem(row, AUTHOR, new QTableWidgetItem(info->author));
    setItem(row, URL, new QTableWidgetItem(info->url));
    setItem(row, PIC, new QTableWidgetItem(info->pic));
    setItem(row, LRC, new QTableWidgetItem(info->lrc));
    QTableWidgetItem* icon_item = new QTableWidgetItem();
    setItem(row, ICON, icon_item);
    // TODO: tidy downloadicon
    // downloadItemIcon(icon_item, info->pic);
}

void TableWidget::insert(int index, QSharedPointer<SongInfo> info)
{
    insertRow(index);
    setRowHeight(index, 50);
    setItem(index, TITLE, new QTableWidgetItem(info->title));
    setItem(index, AUTHOR, new QTableWidgetItem(info->author));
    setItem(index, URL, new QTableWidgetItem(info->url));
    setItem(index, PIC, new QTableWidgetItem(info->pic));
    setItem(index, LRC, new QTableWidgetItem(info->lrc));
    QTableWidgetItem* icon_item = new QTableWidgetItem();
    setItem(index, ICON, icon_item);
    // TODO: tidy downloadicon
    // downloadItemIcon(icon_item, info->pic);
}

void TableWidget::removeRow(QString url)
{
    for (int i = 0; i < rowCount(); )
    {
        if (item(i, URL) && item(i, URL)->text() == url)
            QTableWidget::removeRow(i);
        else
            ++i;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_access(new OnlineRequest(this))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(m_player))
{
    // 加载数据库
    loadMysqlIni();

    // 设置 UI
    ui->setupUi(this);
    setStyleSheet();
    // 设置控件
    initStackWindow();
    // 设置事件回调
    setEventHandler();

    // 加载播放器
    loadMediaPlayer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_systemTray->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::loadMysqlIni()
{
    qDebug() << "========load config start...========";
    QSettings settings("config.ini", QSettings::IniFormat);
    bool ok{false};
    QString db_host = settings.value("MYSQL/Host").toString();
    int db_port = settings.value("MYSQL/Port").toInt(&ok);
    if (!ok) {
    }
    QString db_name = settings.value("MYSQL/Name").toString();
    QString account_name = settings.value("MYSQL/User").toString();
    QString account_pwd = settings.value("MYSQL/Password").toString();
    if (!ok) {
        qCritical() << "========load MYSQL server port error========";
        exit(1);
    }
    qDebug() << "db: " << db_name << "\n"
             << "addr: " << db_host << ":" << db_port << "\n"
             << "user: " << account_name << ":" << account_pwd;
    qDebug() << "========load config end.========";
    m_sql = new LocalSql(db_name, db_host, db_port, account_name, account_pwd, this);
}

void MainWindow::loadMediaPlayer()
{
    m_player->setAudioOutput(m_audio);
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 value) {
        ui->m_sliderMediaTicker->setRange(0, m_player->duration());
        ui->m_sliderMediaTicker->setValue(value);
        ui->m_labMediaTicker->setText(format(value));
        ui->m_labMediaDuration->setText(format(m_player->duration()));
    });
    connect(m_player, &QMediaPlayer::playingChanged, this, &MainWindow::on_playingStopped);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &MainWindow::on_playingError);


    // 播放当前播放列表
    m_currentPlayingIndex = -1;
    if (m_playlistTable->rowCount() > 0) {
        ++m_currentPlayingIndex;
        syncPlay(m_playlistTable->item(0, URL)->text());
    }
}

void MainWindow::setStyleSheet()
{
    // 设置窗口透明背景
    setAttribute(Qt::WA_TranslucentBackground);// 不设置的话，widget 的圆角无法衬托出来
    // 隐藏窗口栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    // 不设置边距
    ui->m_mainWidget->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0,0,0,0);

    ui->m_listWidget->setStyleSheet(
        "QListWidget {"
        "   border: none;"
        "   background: #f5f5f5;"
        "   border-top-left-radius: 10px;"
        "   border-bottom-left-radius: 10px;"
        "}"
        "QListWidget::item {"
        "   height: 40px;"
        "   border-bottom: 1px solid #ddd;"
        "   padding-left: 10px;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #e0e0e0;"
        "}"
        "QListWidget::item:selected {"
        "   background: white;"
        "   color: black;"
        "   border-left: 3px solid #1E90FF;"
        "}"
        );

    ui->m_stackWidget->setStyleSheet(
        "QStackedWidget {"
        "   border-top-right-radius: 15px;"
        "   border-bottom-right-radius: 15px;"
        "   background: white;"
        "}"
        );

    // 设置控件圆角
    QMainWindow::setStyleSheet("QWidget {"
                                    "border-radius: 10px;"  // 圆角半径
                               "}");


    // 设置透明度
    setOpacity(ui->m_listWidget, 0.7);
    setOpacity(ui->m_stackWidget, 0.7);
    setOpacity(ui->m_lineEditSearch, 0.7);

}

void MainWindow::setOpacity(QWidget* widget, qreal opacity)
{
    // 设置控件透明度（0.0-1.0） -- 每个 QGraphicsEffect 对象只能被一个 QWidget 拥有
    // 防止内存泄漏 --> 创建的时候直接关联控件
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(opacity);
    widget->setGraphicsEffect(effect);
}

void MainWindow::setEventHandler()
{
    connect(ui->m_btnMin, &QPushButton::clicked, this, &QMainWindow::showMinimized);
    connect(ui->m_btnMax, &QPushButton::clicked, this, &MainWindow::showMaxOrNormal);
    connect(ui->m_btnClose, &QPushButton::clicked, this, &QMainWindow::close);

    m_dragValidRect = QRect(this->rect().x(),
                            this->rect().y(),
                            this->rect().width(),
                            this->rect().height() -
                                ui->m_hLayoutContent->contentsRect().height() -
                                ui->m_hLayoutBottom->contentsRect().height());

    setMouseTracking(true); // 启用鼠标跟踪
    installEventFilter(this); // 安装事件过滤器

    // 更换皮肤菜单项
    QAction* actionbackToDefault = new QAction(QIcon(), "default", this);
    connect(actionbackToDefault, &QAction::triggered, this, &MainWindow::backtoDefault);
    QAction* actionbackToPink = new QAction(QIcon(), "pink", this);
    connect(actionbackToPink, &QAction::triggered, this, &MainWindow::backtoPink);
    QAction* actionbackToCustom = new QAction(QIcon(), "custom", this);
    connect(actionbackToCustom, &QAction::triggered, this, &MainWindow::backtoCustom);

    m_menuChangeSkin = new QMenu(this);
    m_menuChangeSkin->addAction(actionbackToDefault);
    m_menuChangeSkin->addAction(actionbackToPink);
    m_menuChangeSkin->addAction(actionbackToCustom);
    backtoDefault();
    // 初始化系统托盘
    initSystemTray();
}

void MainWindow::initStackWindow()
{
    ui->m_mainWidget->setUpdatesEnabled(false);
    qDebug() << "init widgets start...";
    QListWidgetItem* likely = new QListWidgetItem(tr("我喜欢的"), ui->m_listWidget);
    QListWidgetItem* local  = new QListWidgetItem(tr("本地音乐"), ui->m_listWidget);
    QListWidgetItem* history = new QListWidgetItem(tr("播放历史"), ui->m_listWidget);
    QListWidgetItem* playlist = new QListWidgetItem(tr("播放列表"), ui->m_listWidget);
    ui->m_listWidget->addItem(likely);
    ui->m_listWidget->addItem(local);
    ui->m_listWidget->addItem(history);
    ui->m_listWidget->addItem(playlist);
    ui->m_listWidget->setItemAlignment(Qt::AlignLeft);
    ui->m_listWidget->setFont(QFont("Microsoft YaHei UI", 16, QFont::Bold));

    QWidget* likely_widget = new QWidget(ui->m_stackWidget);
    QWidget* local_widget = new QWidget(ui->m_stackWidget);
    QWidget* history_widget = new QWidget(ui->m_stackWidget);
    QWidget* playlist_widget = new QWidget(ui->m_stackWidget);
    ui->m_stackWidget->addWidget(likely_widget);
    ui->m_stackWidget->addWidget(local_widget);
    ui->m_stackWidget->addWidget(history_widget);
    ui->m_stackWidget->addWidget(playlist_widget);

    connect(ui->m_listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::on_itemClicked);
    connect(ui->m_listWidget, &QListWidget::itemClicked, this, &MainWindow::on_itemClicked);

    ui->m_listWidget->setCurrentItem(likely);
    ui->m_stackWidget->setCurrentWidget(likely_widget);
    ui->m_btnBackWidget->setEnabled(false);

    // init tables
    for (int i = 0; i < 4; ++i)
    {
        m_tables.append(new TableWidget);
    }
    m_playlistTable = m_tables[3];

    // format widget show
    formatWidget(likely_widget, tr("我喜欢的音乐"), m_tables[0]);
    formatWidget(local_widget, tr("本地音乐库"), m_tables[1]);
    formatWidget(history_widget, tr("历史播放列表"), m_tables[2]);
    formatWidget(playlist_widget, tr("播放列表"), m_playlistTable);
    qDebug() << "init widgets successfully.";

    // load data
    qDebug() << "Load data from database start...";
    m_sql->loadDataFromDB(LIKELIST, [this](QSharedPointer<SongInfo> info) {
        m_tables[0]->append(info);
    });
    m_sql->loadDataFromDB(LOCALIST, [this](QSharedPointer<SongInfo> info) {
        m_tables[1]->append(info);
    });
    m_sql->loadDataFromDB(HISTORYLIST, [this](QSharedPointer<SongInfo> info) {
        m_tables[2]->append(info);
    });
    m_sql->loadDataFromDB(PLAYLIST, [this](QSharedPointer<SongInfo> info) {
        m_tables[3]->append(info);
    });
    for (int i = 0; i < 4; ++i)
    {
        for (int row = 0; row < m_tables[i]->rowCount(); ++row)
        {
            downloadItemIcon(m_tables[i]->item(row, ICON), m_tables[i]->item(row, PIC)->text());
        }
    }
    qDebug() << "Load data from database successfully.";

    ui->m_mainWidget->setUpdatesEnabled(true);
}

void MainWindow::formatWidget(QWidget* widget, QString title, TableWidget* table)
{
    QGridLayout* gridLayout = new QGridLayout(widget);
    if (!title.isEmpty())
    {
        QLabel* label = new QLabel(title, widget);
        label->setFont(QFont("Microsoft YaHei UI", 16, QFont::Bold));
        gridLayout->addWidget(label);
    }

    gridLayout->addWidget(table);
    widget->setLayout(gridLayout);

    connect(table, &QTableWidget::itemDoubleClicked, this, &MainWindow::on_tableItemDoubleClicked);
}

void MainWindow::on_itemClicked(QListWidgetItem *item)
{
    // 如果点击的是 currentWidget，就不做操作了
    if (ui->m_stackWidget->currentIndex() == ui->m_listWidget->currentRow())
        return;

    // 将页面设置为当前点击的这一行对应的页面
    changeWidget(ui->m_listWidget->currentRow());
}

void MainWindow::changeWidget(int new_index)
{
    // 记录上一个页面 index
    int index = ui->m_stackWidget->currentIndex();
    // qDebug() << "changeWidget from " << index << " to " << new_index << " total widgets: " << ui->m_stackWidget->count() << " start...";
    m_history.push(index);

    ui->m_stackWidget->setCurrentIndex(new_index);

    // 更改按钮使能
    if (!m_history.empty() && !ui->m_btnBackWidget->isEnabled())
        ui->m_btnBackWidget->setEnabled(true);

    ui->m_stackWidget->update();
    // qDebug() << "changeWidget from " << index << " to " << new_index << " end";
}

void MainWindow::setIcon(QTableWidgetItem* item, QByteArray icon_data)
{
    QPixmap pixmap;
    QIcon icon;
    if (!pixmap.loadFromData(icon_data))
        icon = QIcon(":/images/search.svg");
    else
        icon = QIcon(pixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    item->setIcon(icon);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    // 创建圆角路径
    QPainterPath path;
    if (!m_isMaximized)
        path.addRoundedRect(rect(), 10, 10);
    else
        path.addRoundedRect(rect(), 0, 0);
    // 设置剪辑路径
    painter.setClipPath(path);
    // 绘制背景图片（缩放适应）
    painter.drawPixmap(rect(), m_background);
    // 绘制半透明边框
    painter.setPen(QPen(QColor(255, 255, 255, 150), 2));
    painter.drawPath(path);

    // 更新当前自定义工具栏窗口位置，方便后面拖动窗口
    m_dragValidRect = QRect(this->rect().x(),
                            this->rect().y(),
                            this->rect().width(),
                            this->rect().height() -
                                ui->m_hLayoutContent->contentsRect().height() -
                                ui->m_hLayoutBottom->contentsRect().height());

    QMainWindow::paintEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // if (ui->m_hLayoutTop->contentsRect().contains(m_pressedPos)) {
        if (m_dragValidRect.contains(event->pos())) {
            // 在工具栏中点击 --> 拖动窗口
            m_pressedPos = event->pos();
            m_isDrag = true;
            // qDebug() << "drag window begin: " << m_isDrag << " " << m_pressedPos;
        } else {
            // 边缘按下 --> 窗口大小变动
            m_resizeEdge = getResizeEdge(event->pos());
            if (m_resizeEdge != None) {
                m_isResizing = true;
                m_dragStartPosition = event->globalPosition().toPoint();
                m_dragStartGeometry = geometry();
                event->accept();
                return;
            }
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDrag) {
        this->move(event->pos() - m_pressedPos + this->pos());
        // qDebug() << "drag window move: " << m_isDrag << " " << m_pressedPos;
    }
    if (!m_isResizing) {
        // 更新鼠标光标形状
        Edge edge = getResizeEdge(event->pos());
        switch (edge) {
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
        }
    } else if (m_isResizing && (event->buttons() & Qt::LeftButton)) {
        // 计算大小调整
        QRect newGeometry = m_dragStartGeometry;
        QPoint delta = event->globalPosition().toPoint() - m_dragStartPosition;
        if (m_resizeEdge & Right) {
            newGeometry.setRight(newGeometry.right() + delta.x());
            if (newGeometry.width() < minimumWidth()) {
                newGeometry.setRight(newGeometry.left() + minimumWidth());
            }
        }
        setGeometry(newGeometry);
        event->accept();
    } else {
        QMainWindow::mouseMoveEvent(event);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDrag = false;
    if (m_isResizing) {
        m_isResizing = false;
        m_resizeEdge = None;
        event->accept();
    } else {
        QMainWindow::mouseReleaseEvent(event);
    }
}

MainWindow::Edge MainWindow::getResizeEdge(const QPoint &pos)
{
    QRect rect = this->rect();
    int edgeSize = qApp->style()->pixelMetric(QStyle::PM_SizeGripSize);

    // 定义8个边缘区域
    bool atTop = pos.y() < edgeSize;
    bool atBottom = pos.y() > rect.height() - edgeSize;
    bool atRight = pos.x() > rect.width() - edgeSize;

    if (atTop && atRight) return TopRight;
    if (atBottom && atRight) return BottomRight;
    if (atRight) return Right;

    return None;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // 确保鼠标离开控件时恢复默认光标
    if (event->type() == QEvent::Leave) {
        if (!m_isResizing) {
            setCursor(Qt::ArrowCursor);
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showMaxOrNormal()
{
    if (m_isMaximized) {
        m_isMaximized = false;
        showNormal();
        ui->m_btnMax->setIcon(QIcon(ICON_MAX));
    } else {
        m_isMaximized = true;
        showMaximized();
        ui->m_btnMax->setIcon(QIcon(ICON_NORMAL));
    }
    // 这里有问题第一次点击maxicon --> 预期最大化窗口maximum --> 实际一致，且图标变为 normal
    // 1  ismax:  false click or not click
    // 2  ismax:  false click or not click
    // 第二次点击 maxicon --> 预期恢复窗口normal --> 实际：没有动静 输出以下信息：
    // 1  ismax:  false click or not click
    // 2  ismax:  true click or not click
    // 第三次点击 maxicon --> 恢复窗口到 normal 了，且图标变为 max
    // 1  ismax:  true click or not click
    // 2  ismax:  false click or not click
    // 分析：
    // 第一次点击： 来不及处理窗口内部状态
        // 当调用 showMaximized() 后立即调用 isMaximized() 可能会返回旧值，这是因为：
            // ​异步操作​：窗口状态变化是异步的，操作系统需要时间处理请求
            // ​事件循环延迟​：Qt 需要等待系统事件才能更新内部状态
            // ​时序问题​：函数调用立即返回，而实际状态还未改变
    // 第二次点击： 到输出 "2 ismax..." 的时候，事件更新到了 ismaximized 这里了，即窗口状态变化更改了
    // 由于系统的变化太慢，这里采用内部变量适应
    // qDebug() << "1  ismax: " << isMaximized() << "click or not click";
    // if (this->isMaximized()) {
    //     showNormal();
    //     ui->m_btnMax->setIcon(QIcon(ICON_MAX));
    // }
    // else {
    //     showMaximized();
    //     ui->m_btnMax->setIcon(QIcon(ICON_NORMAL));
    // }
    // QApplication::processEvents();
    // qDebug() << "2  ismax: " << isMaximized() << "click or not click";
}

void MainWindow::on_tableItemDoubleClicked(QTableWidgetItem* item)
{
    if (!item) return;

    QSharedPointer<SongInfo> info = TableWidget::getItemInfo(item);
    if (!info) return;

    bool in_playlist = false;
    for (int i = 0; i < m_playlistTable->rowCount(); ++i)
    {
        if (m_playlistTable->item(i, TITLE)->text() == info->title &&
            m_playlistTable->item(i, URL)->text()  == info->url &&
            m_playlistTable->item(i, AUTHOR)->text() == info->author)
        {
            m_currentPlayingIndex = i;
            in_playlist = true;
            break;
        }
    }
    if (!in_playlist)
    {
        // 加入到播放列表中
        m_playlistTable->insert(++m_currentPlayingIndex, info);
        downloadItemIcon(m_playlistTable->item(m_currentPlayingIndex, ICON), info->pic);
        // 加入当前播放列表数据库中
        m_sql->insertIntoPlaylist(info);
        qDebug() << __func__ << " insert playlist>>> index=" << m_currentPlayingIndex << " title=" << getCurrentSong()->title;
    }

    // 播放
    if (m_player->isPlaying()) {
        m_player->pause(); // stop 的话会触发信号状态改变，导致 m_currentPlayingIndex 改变了
    }
    syncPlay(info->url);

    // 存入历史播放列表
    m_sql->updatePlayHistory(info);
    qDebug() << __func__ << " changed current song index=" << m_currentPlayingIndex << " title=" << info->title;
}

void MainWindow::decodeFromJson(QWidget* search_widget, QByteArray response)
{
    qDebug() << "decode from json start...";
    // 尝试解析为JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        // qDebug() << "JSON解析成功:" << jsonDoc.toJson(QJsonDocument::Indented);
        if (jsonDoc.isObject()) {
            QJsonObject obj = jsonDoc.object();
            if (obj.contains("data")) {
                QJsonObject data = obj.value("data").toObject();
                if (data.contains("list")) {
                    // 解析数据到数据库和表中
                    TableWidget* table = Q_NULLPTR;
                    try {
                        if(auto iter = m_searchRes.find(search_widget); iter != m_searchRes.end())
                            table = iter.value();
                        else {
                            qWarning() << "this page has deleted.";
                            disconnect(m_access, &OnlineRequest::PostRequestFinished, this, &MainWindow::decodeFromJson);
                            return;
                        }
                        // TODO： 界面在下载 icon 时卡死
                        table->setUpdatesEnabled(false);
                        QJsonArray array = data.value("list").toArray();
                        for (auto i = 0; i < array.count(); ++i) {
                            QSharedPointer<SongInfo> song(new SongInfo);
                            QJsonObject info = array.at(i).toObject();
                            if (info.contains("title")) {
                                song->title = info.value("title").toString();
                                song->title.replace("'", "’");                // 数据库插入语法错误匹配单引号字符串
                            }
                            if (info.contains("author")) {
                                song->author = info.value("author").toString();
                                song->author.replace("'", "’");
                            }
                            if (info.contains("url"))
                                song->url = info.value("url").toString();
                            if (info.contains("pic"))
                                song->pic = info.value("pic").toString();
                            if (info.contains("lrc")) {
                                song->lrc = info.value("lrc").toString();
                                song->lrc.replace("'", "’");
                            }

                            // 存储到当前页面
                            table->append(song);
                            downloadItemIcon(table->item(table->rowCount()-1, ICON), song->pic);
                        }
                        table->setUpdatesEnabled(true);
                    } catch(...) {
                        qWarning() << "Error updating table";
                        if (table) table->setUpdatesEnabled(true);
                        disconnect(m_access, &OnlineRequest::PostRequestFinished, this, &MainWindow::decodeFromJson);
                    }
                }
            }
        }
        qDebug() << "parse json successfully.";
    } else {
        qDebug() << "parse json failed:" << parseError.errorString();
    }
    disconnect(m_access, &OnlineRequest::PostRequestFinished, this, &MainWindow::decodeFromJson);
}

void MainWindow::downloadItemIcon(QTableWidgetItem* item, const QString iconUrl) {
    m_access->downloadItemIcon(item, iconUrl);
    connect(m_access, &OnlineRequest::itemIconDownloadFinished, this, &MainWindow::setIcon);
}

void MainWindow::syncPlay(QString source)
{
    qDebug() << "syncPlay start index=" << m_currentPlayingIndex << " source=" << source << " start...";
    m_player->setSource(QUrl(source));
    m_player->play();

    QSharedPointer<SongInfo> info = getCurrentSong();

    if (m_sql->isLiked(info))
        ui->m_btnMediaLove->setIcon(QIcon(ICON_LOVED));
    else
        ui->m_btnMediaLove->setIcon(QIcon(ICON_TOLOVE));

    m_access->downloadItemIcon(info->pic);
    connect(m_access, &OnlineRequest::iconDownloadFinished, [this](QByteArray imageData) {
        QPixmap pixmap;
        QIcon icon;
        QSize iconSize(80, 80);
        if (!pixmap.loadFromData(imageData))
            icon = QIcon(":/images/search.svg");
        else
            icon = QIcon(pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QMetaObject::invokeMethod(this, [this, icon]() {
            this->ui->m_btnMediaFace->setIcon(icon);
            this->ui->m_btnMediaFace->setIconSize(QSize(80,80));
        }, Qt::QueuedConnection);
    });

    // 不同步，m_playlistTable 和 m_currentPlayIndex
    // TODO: 插入到当前播放列表也不同步

    ui->m_btnGoPlay->setIcon(QPixmap(ICON_TOPAUSE));
    ui->m_btnMediaName->setText(info->title);
    ui->m_btnMediaPlayer->setText(info->author);
    m_playlistTable->setCurrentItem(m_playlistTable->item(m_currentPlayingIndex, TITLE));

    qDebug() << "syncPlay start index=" << m_currentPlayingIndex << " source=" << source << " end.";
}

void MainWindow::on_playingStopped(bool playing)
{
    if (playing == false && m_player->playbackState() == QMediaPlayer::StoppedState) {
        if (m_playlistTable->rowCount() == 0) return;
        ++m_currentPlayingIndex;
        if (m_currentPlayingIndex >= m_playlistTable->rowCount()) {
            m_currentPlayingIndex = 0;
        }
        syncPlay(m_playlistTable->item(m_currentPlayingIndex, URL)->text());

        QSharedPointer<SongInfo> info = getCurrentSong();
        m_sql->updatePlayHistory(info);
    }
}

void MainWindow::on_playingError(QMediaPlayer::Error error, const QString &errorString)
{
    qWarning() << "mediaplay occurs error: " << errorString << " source=" << m_player->source() << " current_index=" << m_currentPlayingIndex  << " now count=" << m_playlistTable->rowCount();

    QString source = m_player->source().toString();

    qDebug() << "on_playingError remove for source=" << source << " start...";

    m_playlistTable->removeRow(source);

    m_sql->deleteFromPlaylist(source);

    qDebug() << "on_playingError remove for source=" << source << " now=" << m_playlistTable->item(m_currentPlayingIndex, URL)->text() << " current_index=" << m_currentPlayingIndex << " now count=" << m_playlistTable->rowCount() << " end.";
}

void MainWindow::on_m_btnMain_clicked()
{
    // 如果当前页面已经是第一个页面，就不需要操作了
    if (ui->m_stackWidget->currentIndex() == 0) return;

    // 切换到第一个页面
    changeWidget(0);
}

void MainWindow::on_m_btnBackWidget_clicked()
{
    if (m_history.empty()) return;

    // 获取历史
    int index = m_history.top();
    if (index == -1)
        return;
    m_history.pop();

    // 切换页面
    int old_index = ui->m_stackWidget->currentIndex();
    ui->m_stackWidget->setCurrentIndex(index);

    // 修改按钮使能标志
    if (m_history.empty() && ui->m_btnBackWidget->isEnabled())
        ui->m_btnBackWidget->setEnabled(false);

    // 是否删除页面缓存
    if (auto old_iter = m_searchHistory.find(old_index); old_iter != m_searchHistory.end())
    {
        if (!m_history.contains(old_index))
        {
            QWidget* widget = old_iter.value();
            ui->m_stackWidget->removeWidget(widget);
            m_searchHistory.erase(old_iter);
            widget->deleteLater();
        }
    }
}

void MainWindow::on_m_btnSearch_clicked()
{
    if (ui->m_lineEditSearch->text().isEmpty()) return;

    // 界面更新
    ui->m_stackWidget->setUpdatesEnabled(false);
    // 添加新页面
    QWidget* search_widget = new QWidget(ui->m_stackWidget);
    ui->m_stackWidget->addWidget(search_widget);
    TableWidget* search_table = new TableWidget(search_widget);
    // 必须先设置足够的行数和列数,否则不显示
    search_table->setColumnCount(6);
    formatWidget(search_widget, tr("搜索结果："), search_table);
    m_searchHistory.insert(ui->m_stackWidget->currentIndex(), search_widget);
    m_searchRes.insert(search_widget, search_table);
    // 更换页面
    m_history.push(ui->m_stackWidget->currentIndex());
    ui->m_stackWidget->setCurrentWidget(search_widget);
    ui->m_stackWidget->setUpdatesEnabled(true);
    // 更改按钮使能
    if (!m_history.empty() && !ui->m_btnBackWidget->isEnabled())
        ui->m_btnBackWidget->setEnabled(true);
    QString input = ui->m_lineEditSearch->text();

    // post 请求网络数据
    m_access->RequestPost(search_widget, input);
    connect(m_access, &OnlineRequest::PostRequestFinished, this, &MainWindow::decodeFromJson);
}

void MainWindow::on_m_btnInfo_clicked()
{

}

void MainWindow::on_m_btnNotify_clicked()
{

}

void MainWindow::on_m_btnSetting_clicked()
{

}

void MainWindow::on_m_btnSkin_clicked()
{
    m_menuChangeSkin->exec(QCursor::pos());
}

void MainWindow::on_m_btnMini_clicked()
{

}

void MainWindow::on_m_btnMediaLove_clicked()
{
    QSharedPointer<SongInfo> info = getCurrentSong();

    if (!m_sql->isLiked(info)) {
        QString res = m_sql->insertLikeList(info);
        if (res.contains("已")) {
            ui->m_btnMediaLove->setIcon(QIcon(ICON_LOVED));
        }
        QLabel* label = new QLabel(res, this);
        label->setWordWrap(true);
        label->move(QPoint(this->rect().x() + this->rect().width()/2, this->rect().y() + this->rect().height()/2));
        label->show();
        QTimer::singleShot(3000, [this,label]{
            label->hide();
            label->deleteLater();
            qDebug() << "delete label;";
        });
        m_tables[0]->append(info);
    } else {
        m_sql->deleteFromLikelist(info);
        m_tables[0]->removeRow(info->url);
        ui->m_btnMediaLove->setIcon(QIcon(ICON_TOLOVE));
    }
}

void MainWindow::on_m_btnGoPrevious_clicked()
{
    if (m_playlistTable->rowCount() > 0) {
        m_currentPlayingIndex = ((m_currentPlayingIndex <= 0) ? m_playlistTable->rowCount() : m_currentPlayingIndex)-1;
        if (m_player->isPlaying()) {
            m_player->pause();
        }
        syncPlay(m_playlistTable->item(m_currentPlayingIndex, URL)->text());
    }
}

void MainWindow::on_m_btnGoPlay_clicked()
{
    if (m_player->isPlaying()) {
        m_player->pause();
        ui->m_btnGoPlay->setIcon(QPixmap(ICON_TOPLAY));
    } else if(m_player->playbackState() == QMediaPlayer::PausedState) {
        m_player->play();
        ui->m_btnGoPlay->setIcon(QPixmap(ICON_TOPAUSE));
    }
}

void MainWindow::on_m_btnGoNext_clicked()
{
    if (m_playlistTable->rowCount() > 0) {
        m_currentPlayingIndex = (m_currentPlayingIndex < m_playlistTable->rowCount()) ? (m_currentPlayingIndex+1) : 0;
        if (m_player->isPlaying()) {
            m_player->pause();
        }
        syncPlay(m_playlistTable->item(m_currentPlayingIndex, URL)->text());
    }
}

void MainWindow::on_m_btnMediaLoop_clicked()
{
    // show loop
}

void MainWindow::on_m_btnMediaLyric_clicked()
{
    // show current lyric;
}

void MainWindow::on_m_btnMediaSound_clicked()
{
    // m_audio->setVolume(0.5);
}

void MainWindow::on_m_sliderMediaTicker_sliderMoved(int position)
{
    if (!m_player->isPlaying()) {
        return;
    }

    m_player->setPosition(position);
}

void MainWindow::backtoDefault()
{
    qDebug() << "default background setting start...";
    m_background.load(BG_DEFAULT);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(m_background.scaled(
                         this->size(), Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation)));
    this->update();
    qDebug() << "default background setting successfully.";
}

void MainWindow::backtoPink()
{
    qDebug() << "pink background setting start...";
    m_background.load(BG_PINK);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(m_background.scaled(
                         this->size(), Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation)));
    this->update();
    qDebug() << "pink background setting successfully.";
}

void MainWindow::backtoCustom()
{
    qDebug() << "custom background setting start...";
    // 选择打开图片作为皮肤
    QString strFileName = QFileDialog::getOpenFileName(this, tr("请选择自定义背景图片"),
                                                       QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first(),u8"(*.jpg *.jpeg *.png)");

    m_background.load(strFileName);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(m_background.scaled(
                         this->size(), Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation)));
    this->update();
    qDebug() << "custom background setting successfully.";
}

// 响应系统托盘双击操作
void MainWindow::on_systemTray(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason){
    case QSystemTrayIcon::Trigger:
        if (isHidden()) {
            this->show();
            this->activateWindow();
        }
        else
            hide();
        break;
    case QSystemTrayIcon::DoubleClick: {
        if (this->isActiveWindow()) {
            this->show();
            this->activateWindow();
        }
    }
    break;
    case QSystemTrayIcon::Context:
    default:
        break;
    }
}
// 初始化系统托盘
void MainWindow::initSystemTray()
{
    m_systemTray = new QSystemTrayIcon(this);
    m_systemTray->setIcon(QIcon(ICON_LOGO));
    connect(m_systemTray, &QSystemTrayIcon::activated, this, &MainWindow::on_systemTray);

    QAction* actionQuit = new QAction(tr(u8"退出"), this);
    connect(actionQuit, &QAction::triggered, this, &MainWindow::quitSystemTray);

    QMenu* pcontextmenu = new QMenu(this);
    pcontextmenu->addAction(actionQuit);
    m_systemTray->setContextMenu(pcontextmenu);

    m_systemTray->show();
}
// 退出应用程序
void MainWindow::quitSystemTray()
{
    QCoreApplication::quit();
}
// 获取当前播放的音乐信息
QSharedPointer<SongInfo> MainWindow::getCurrentSong()
{
    QSharedPointer<SongInfo> info(new SongInfo);
    info->title = m_playlistTable->item(m_currentPlayingIndex, TITLE)->text();
    info->author = m_playlistTable->item(m_currentPlayingIndex, AUTHOR)->text();
    info->url = m_playlistTable->item(m_currentPlayingIndex, URL)->text();
    info->pic = m_playlistTable->item(m_currentPlayingIndex, PIC)->text();
    info->lrc = m_playlistTable->item(m_currentPlayingIndex, LRC)->text();
    qDebug() << "m_currentplay index=" << m_currentPlayingIndex << " title=" << info->title << " author=" << info->author;
    return info;
}
