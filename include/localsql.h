#ifndef LOCALSQL_H
#define LOCALSQL_H

#include <QObject>
#include <QSqlRecord>

class QSqlDatabase;
class QTableWidget;
struct SongInfo;

class LocalSql : public QObject
{
    Q_OBJECT
public:
    explicit LocalSql(QString db_name,
                      QString db_host, int db_port,
                      QString account_name, QString account_pwd,
                      QObject *parent = nullptr);

    // 数据表的 CRUD 操作
    bool sqlQuery(QString sql);

    // 从数据库中加载数据到 tablewidget 中，图标链接放在第三个参数中
    void loadDataFromDB(QString table_name, std::function<void(QSharedPointer<SongInfo>)> appendRowItem);

    // 添加到当前播放列表
    void insertIntoPlaylist(QSharedPointer<SongInfo>);
    // 删除播放列表中的一项
    bool deleteFromPlaylist(QString url);
    // 更新播放历史列表
    void updatePlayHistory(QSharedPointer<SongInfo>);

    // 添加喜欢的音乐
    bool isLiked(QSharedPointer<SongInfo>);
    QString insertLikeList(QSharedPointer<SongInfo>);
    bool deleteFromLikelist(QSharedPointer<SongInfo>);
private:
    // 打开数据库
    bool initDatabase(QString db_name,
                      QString db_host, int db_port,
                      QString account_name, QString account_pwd);
    // 加载数据库表
    bool loadDatabaseTables();

signals:

private:
    QSqlDatabase* m_db;
};

#endif // LOCALSQL_H
