#ifndef USERSESSION_H
#define USERSESSION_H

#include <QString>

class UserSession
{
public:
    static UserSession* getInstance();
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
