#include "../include/localsql.h"
#include "../include/utils.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QProcess>

#include <QTableWidget>

LocalSql::LocalSql(QString db_name,
                   QString db_host, int db_port,
                   QString account_name, QString account_pwd,
                   QObject *parent)
    : QObject{parent}
{
    if (!initDatabase(db_name, db_host, db_port, account_name, account_pwd))
        exit(EXIT_SQL_INIT_ERROR);
    if (!loadDatabaseTables())
        exit(EXIT_SQL_LOAD_ERROR);
}

bool LocalSql::sqlQuery(QString sql)
{
    QSqlQuery query;
    if (query.exec(sql))
        return true;
    qCritical() << "SQL exec failed: " << m_db->lastError().text() << " sql=" << sql;
    return false;
}

void LocalSql::loadDataFromDB(QString table_name, std::function<void(QSharedPointer<SongInfo>)> appendRowItem)
{
    QString sql = QString("select * from %1").arg(table_name);
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(sql);

    if (!query.exec())
    {
        qCritical() << "History table query failed: " << m_db->lastError().text();
        return;
    }
    while (query.next()) {
        QSqlRecord rec = query.record();

        QSharedPointer<SongInfo> info(new SongInfo);
        info->title = rec.value("title").toString();
        info->author = rec.value("author").toString();
        info->pic = rec.value("pic").toString();
        info->lrc = rec.value("lrc").toString();
        info->url = rec.value("url").toString();

        appendRowItem(info);
    }
}

void LocalSql::insertIntoPlaylist(QSharedPointer<SongInfo> info)
{
    QString sql = QString("INSERT INTO %1 VALUES(null, '%2', '%3', '%4', '%5', '%6');").
          arg(PLAYLIST, info->title, info->author, info->url, info->pic, info->lrc);
    sqlQuery(sql);
}

bool LocalSql::deleteFromPlaylist(QString url)
{
    QString sql = QString("DELETE FROM %1 WHERE url='%2'").
                  arg(PLAYLIST, url);
    return sqlQuery(sql);
}

void LocalSql::updatePlayHistory(QSharedPointer<SongInfo> info)
{
    QString sql = QString("DELETE FROM %1 WHERE title='%2' AND author='%3' AND url='%4'").
                  arg(HISTORYLIST, info->title, info->author, info->url);
    if (!sqlQuery(sql))
        return;

    sql = QString("INSERT INTO %1 VALUES(null, '%2', '%3', '%4', '%5', '%6');").
          arg(HISTORYLIST, info->title, info->author, info->url, info->pic, info->lrc);
    sqlQuery(sql);
}

bool LocalSql::isLiked(QSharedPointer<SongInfo> info)
{
    QString sql = QString("SELECT id FROM %1 WHERE title='%2' AND author='%3' AND url= '%4'").
                  arg(LIKELIST, info->title, info->author, info->url);
    QSqlQuery query;
    if (query.exec(sql)) {
        if (query.size() > 0)
            return true;
    } else {
        qWarning() << tr("数据库查询失败： ") << m_db->lastError().text();
    }
    return false;
}

QString LocalSql::insertLikeList(QSharedPointer<SongInfo> info)
{
    QString sql = QString("SELECT title FROM %1 WHERE title='%2' AND author='%3' AND url= '%4'").
                  arg(LIKELIST, info->title, info->author, info->url);
    QSqlQuery query;
    if (query.exec(sql)) {
        if (query.size() > 0) {
            return QString(tr("已经添加过了"));
        } else {
            sql = QString("INSERT INTO %1 VALUES(null, '%2', '%3', '%4', '%5', '%6');").
                  arg(LIKELIST, info->title, info->author, info->url, info->pic, info->lrc);
            if (sqlQuery(sql)) {
                return QString(tr("已添加"));
            }
        }
    }
    return QString("添加失败： " + m_db->lastError().text());
}

bool LocalSql::deleteFromLikelist(QSharedPointer<SongInfo> info)
{
    QString sql = QString("DELETE FROM %1 WHERE title='%2' AND author='%3' AND url='%4'").
                  arg(LIKELIST, info->title, info->author, info->url);
    return sqlQuery(sql);
}

bool LocalSql::initDatabase(QString db_name,
                            QString db_host, int db_port,
                            QString account_name, QString account_pwd)
{
    qDebug() << "initDatabase start...";
    QProcess process;
    QStringList args;
    args << "-u" << account_name
         << QString("-p%1").arg(account_pwd)
         << "-e" << QString("CREATE DATABASE IF NOT EXISTS %1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci").arg(db_name);

    process.start("mysql", args);
    if (!process.waitForFinished(5000)) {
        qCritical() << "Process Create Database failed:" << process.errorString();
        return false;
    }

    if (process.exitCode() != 0) {
        qCritical() << "Create Database failed: " << process.readAllStandardError();
        return false;
    }
    m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL"));
    m_db->setDatabaseName(db_name);
    m_db->setHostName(db_host);
    m_db->setPort(db_port);
    m_db->setUserName(account_name);
    m_db->setPassword(account_pwd);
    if (!m_db->open())
    {
        qDebug() << "Open database music_api_db failed: " << m_db->lastError().text();
        return false;
    }
    qDebug() << "initDatabase successfully.";
    return true;
}

bool LocalSql::loadDatabaseTables()
{
    qDebug() << "loadDatabaseTables start...";
    if (!sqlQuery(QString("CREATE TABLE IF NOT EXISTS %1("
                          "id integer, title text, author text, url text, pic text, lrc text);").arg(LIKELIST)))
        return false;
    if (!sqlQuery(QString("CREATE TABLE IF NOT EXISTS %1("
                          "id integer primary key auto_increment, title text, author text, url text, pic text, lrc text);").arg(HISTORYLIST)))
        return false;
    if (!sqlQuery(QString("CREATE TABLE IF NOT EXISTS %1("
                          "id integer primary key auto_increment, title text, author text, url text, pic text, lrc text);").arg(LOCALIST)))
        return false;
    if (!sqlQuery(QString("CREATE TABLE IF NOT EXISTS %1("
                          "id integer, title text, author text, url text, pic text, lrc text);").arg(PLAYLIST)))
        return false;
    if (!sqlQuery(QString("CREATE TABLE IF NOT EXISTS %1("
                          "id integer, title text, author text, url text, pic text, lrc text);").arg(SEARCHLIST)))
        return false;

    qDebug() << "loadDatabaseTables successfully.";
    return true;
}

