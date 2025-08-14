#pragma once

#include <QString>
#include <QHash>

struct SongInfo {
    QString title;
    QString author;
    QString url;
    QString pic;
    QString lrc;
};

extern QString LIKELIST;
extern QString LOCALIST;
extern QString HISTORYLIST;
extern QString PLAYLIST;
extern QString SEARCHLIST;

extern QHash<int, QString> TABLES;

// 学习使用
extern QString API_SEARCH;

// 进程图标
extern QString ICON_LOGO;

// 退出码
extern int EXIT_SQL_INIT_ERROR;
extern int EXIT_SQL_LOAD_ERROR;

QString format(qint64 ms);
