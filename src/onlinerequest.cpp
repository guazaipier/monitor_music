#include "../include/onlinerequest.h"
#include "../include/utils.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

OnlineRequest::OnlineRequest(QObject *parent)
    : QObject{parent}
    , m_access(new QNetworkAccessManager(this))
    , m_reply(Q_NULLPTR)
{}

void OnlineRequest::RequestPost(QWidget* table, QString input)
{
    // post 请求网络数据
    QNetworkRequest request(API_SEARCH);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded; charset=UTF-8");
    request.setRawHeader("accept", "*/*");
    request.setRawHeader("accept-language", "zh-CN,zh;q=0.9");
    request.setRawHeader("priority", "u=1, i");
    request.setRawHeader("sec-ch-ua", "\"Not)A;Brand\";v=\"8\", \"Chromium\";v=\"138\", \"Google Chrome\";v=\"138\"");
    request.setRawHeader("sec-ch-ua-mobile", "?0");
    request.setRawHeader("sec-ch-ua-platform", "\"Windows\"");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "cors");
    request.setRawHeader("sec-fetch-site", "same-origin");
    request.setRawHeader("x-requested-with", "XMLHttpRequest");
    // 准备POST数据
    QUrlQuery postData;
    postData.addQueryItem("input", input);
    postData.addQueryItem("filter", "name");
    postData.addQueryItem("page", "1");
    postData.addQueryItem("type", "netease");
    QByteArray postDataEncoded = postData.toString(QUrl::FullyEncoded).toUtf8();
    // 发送POST请求
    m_reply = m_access->post(request, postDataEncoded);
    connect(m_reply, &QNetworkReply::finished, this, [this, table] {
        if (m_reply->error() == QNetworkReply::NoError) {
            QByteArray response = m_reply->readAll();
            qDebug() << "原始响应长度: " << response.length();
            emit PostRequestFinished(table, response);
        } else {
            qDebug() << "网络请求错误:" << m_reply->errorString();
            qDebug() << "HTTP状态码:" << m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        m_reply->deleteLater();
    });
}

void OnlineRequest::downloadItemIcon(const QString iconUrl)
{
    QNetworkReply *reply = m_access->get(QNetworkRequest(iconUrl));
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "图标下载失败:" << reply->errorString();
            return;
        }

        // 处理下载的图片数据
        QByteArray imageData = reply->readAll();
        emit iconDownloadFinished(imageData);
    });
}

void OnlineRequest::downloadItemIcon(QTableWidgetItem* item, const QString iconUrl)
{
    QNetworkReply *reply = m_access->get(QNetworkRequest(iconUrl));
    connect(reply, &QNetworkReply::finished, [this, reply, item]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "图标下载失败:" << reply->errorString();
            return;
        }

        // 处理下载的图片数据
        QByteArray imageData = reply->readAll();
        emit itemIconDownloadFinished(item, imageData);
    });
}
