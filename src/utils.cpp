#include "../include/utils.h"

QString LIKELIST    = "likelist_tb";
QString LOCALIST    = "locallist_tb";
QString HISTORYLIST = "historylist_tb";
QString PLAYLIST    = "playlist_tb";
QString SEARCHLIST  = "searchlist_tb";

QHash<int, QString> TABLES = {{0, LIKELIST},
                              {1, LOCALIST},
                              {2, HISTORYLIST},
                              {3, PLAYLIST}};

// 学习使用
QString API_SEARCH = "https://www.myfreemp3.com.cn/";

// 进程图标
QString ICON_LOGO = ":/images/logo.png";

// 退出码
int EXIT_SQL_INIT_ERROR = 1;
int EXIT_SQL_LOAD_ERROR = 2;

QString format(qint64 ms) {
    static const qint64 MS_PER_HOUR = 3600000;
    static const qint64 MS_PER_MINUTE = 60000;
    static const qint64 MS_PER_SECOND = 1000;

    qint64 hours = ms / MS_PER_HOUR;
    ms %= MS_PER_HOUR;
    qint64 minutes = ms / MS_PER_MINUTE;
    ms %= MS_PER_MINUTE;
    qint64 seconds = ms / MS_PER_SECOND;

    if (hours > 0) {
        return QString("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}
