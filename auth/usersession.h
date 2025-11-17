#ifndef USERSESSION_H
#define USERSESSION_H

#include <QString>
#include <QCryptographicHash>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include "service/clientservice.h"
#include "db/dbconnector.h"

class UserSession
{
public:
    static UserSession* getInstance();
    int login(const QString& username, const QString& password);
    int saveUserPassword(ClientService* service, const QString& username, const QString& password,
                          const QString& address,
                          const QString& bossName, const QString& bossPhone,
                          const QString& accountantName, const QString& accountantPhone);
    void createSession(int userId, const QString& username);
    void destroySession();
    int getUserId() const;
    QString getUsername() const;
    bool isLoggedIn() const;

private:
    UserSession();
    UserSession(const UserSession&) = delete;
    UserSession& operator=(const UserSession&) = delete;

    int m_userId;
    QString m_username;
    bool m_isLoggedIn;
};

#endif // USERSESSION_H
