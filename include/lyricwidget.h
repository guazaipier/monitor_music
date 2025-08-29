#ifndef LYRICWIDGET_H
#define LYRICWIDGET_H


#include <QWidget>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QListWidget>

class LyricWidget : public QListWidget {
    Q_OBJECT
    // Q_PROPERTY 宏详解：理解 Qt 属性系统, 这个语法用于在 Qt 的元对象系统中注册一个属性
    // Q_PROPERTY(type name READ getFunction [WRITE setFunction] [NOTIFY signal] [OTHER options])
    Q_PROPERTY(int scrollPosition READ scrollPosition WRITE setScrollPosition)
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged
                   USER true)
    Q_PROPERTY(bool sortingEnabled READ isSortingEnabled WRITE setSortingEnabled)


public:
    explicit LyricWidget(QWidget *parent = nullptr);

    // 设置歌词到 listwidget 中
    void setLyric(const QString &lyric);

    // 根据播放进度设置当前歌词对应的位置
    void setCurrentPosition(qint64 position);

    // 读取函数：返回滚动位置
    int scrollPosition() const;
    // 写入函数：设置滚动位置
    void setScrollPosition(int position);

private:
    // 更新当前播放的歌词
    void updateHighlight();

    // 动态滚动到当前行
    void scrollToCurrent();

private:
    QVector<QPair<qint64, QString>> m_lyrics;
    int m_currentIndex, m_lastIndex;
    QPropertyAnimation *m_scrollAnimation;
    QFont m_font, m_focusFont;
};

#endif // LYRICWIDGET_H
