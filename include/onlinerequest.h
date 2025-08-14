#ifndef ONLINEREQUEST_H
#define ONLINEREQUEST_H

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QTableWidget;
class QTableWidgetItem;

class OnlineRequest : public QObject
{
    Q_OBJECT
public:
    explicit OnlineRequest(QObject *parent = nullptr);

    // 通过 post 请求搜索歌曲，finished 后触发 PostRequestFinished 信号
    void RequestPost(QWidget*, QString input);
    // 通过 get 请求获取歌曲图标，finished 后触发 iconDownloadFinished 信号
    void downloadItemIcon(const QString iconUrl);
    // 绑定 icon 设置的位置
    void downloadItemIcon(QTableWidgetItem*, const QString iconUrl);

signals:
    void PostRequestFinished(QWidget*, QByteArray bytes);
    void iconDownloadFinished(QByteArray bytes);
    void itemIconDownloadFinished(QTableWidgetItem*, const QByteArray bytes);
private:
    QNetworkAccessManager* m_access;
    QNetworkReply* m_reply;
};

#endif // ONLINEREQUEST_H
