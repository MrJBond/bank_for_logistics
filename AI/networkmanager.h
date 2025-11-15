#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    NetworkManager(QObject *parent = nullptr) : QObject(parent) {
        m_manager = new QNetworkAccessManager(this);
    }
protected:
    QNetworkAccessManager *m_manager;
signals:
    void networkError(const QString& errorString);
};

#endif // NETWORKMANAGER_H
