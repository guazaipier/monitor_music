#include "../include/lyricwidget.h"

LyricWidget::LyricWidget(QWidget *parent)
    : QListWidget(parent)
    , m_currentIndex(-1)
    , m_lastIndex(-1)
    , m_scrollAnimation(new QPropertyAnimation(this, "scrollPosition", this))
    , m_font(QFont("Microsoft YaHei UI", 10))
    , m_focusFont(QFont("Microsoft YaHei UI", 15))
{

    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);

    // 设置样式
    setStyleSheet(
        "QListWidget {"
        "   background: transparent;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item:selected, QListWidget::item:hover {"
        "   background: transparent;"
        "}"
        );

    setFont(m_font);

    // 设置滚动动画
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_scrollAnimation->setDuration(500);
}

void LyricWidget::setLyric(const QString &lyric)
{
    m_lyrics.clear();

    auto lyrics = lyric.split("\n");
    QRegularExpression timeRegex("\\[(\\d+):(\\d+)\\.(\\d+)\\]");
    for (auto line : lyrics)
    {
        QRegularExpressionMatch match = timeRegex.match(line);

        if (match.hasMatch()) {
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            int milliseconds = match.captured(3).toInt();

            // 计算总毫秒数
            qint64 totalMs = minutes * 60000 + seconds * 1000 + milliseconds * 10;

            // 提取歌词文本
            QString lyricText = line.mid(match.capturedLength());

            m_lyrics.append(qMakePair(totalMs, lyricText));
        }
    }

    // 将歌词设置到 widget 中
    this->clear();
    for (const auto &lyric : m_lyrics) {
        QListWidgetItem *item = new QListWidgetItem(lyric.second);
        item->setTextAlignment(Qt::AlignCenter);
        addItem(item);
    }
}

void LyricWidget::setCurrentPosition(qint64 position) {
    // 查找当前歌词索引
    int newIndex = -1;
    for (int i = 0; i < m_lyrics.size(); ++i) {
        if (m_lyrics[i].first <= position) {
            newIndex = i;
        } else {
            break;
        }
    }

    if (newIndex != m_currentIndex && newIndex >= 0) {
        m_currentIndex = newIndex;
        updateHighlight();
        scrollToCurrent();
    }
}

int LyricWidget::scrollPosition() const { return verticalScrollBar()->value(); }

void LyricWidget::setScrollPosition(int position) { verticalScrollBar()->setValue(position); }

void LyricWidget::updateHighlight() {
    for (int i = 0; i < count(); ++i) {
        this->item(i)->setSelected(i == m_currentIndex);
        if (i == m_currentIndex) {
            this->item(i)->setFont(m_focusFont);
        } else {
            this->item(i)->setFont(m_font);
        }

    }
}

void LyricWidget::scrollToCurrent() {
    if (m_currentIndex < 0) return;

    // 计算目标滚动位置
    int itemHeight = sizeHintForRow(0);
    int targetPosition = m_currentIndex * itemHeight - height() / 2 + itemHeight / 2;

    // 设置动画目标值
    m_scrollAnimation->setStartValue(scrollPosition());
    m_scrollAnimation->setEndValue(targetPosition);
    m_scrollAnimation->start();
}
