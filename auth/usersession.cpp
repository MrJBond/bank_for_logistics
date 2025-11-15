#include "usersession.h"

UserSession* UserSession::getInstance()
{
    // Thread-safe static instance
    static UserSession instance;
    return &instance;
}

UserSession::UserSession()
    : m_userId(0), m_isLoggedIn(false)
{
    // Default state is logged out
}

void UserSession::createSession(int userId, const QString& username)
{
    m_userId = userId;
    m_username = username;
    m_isLoggedIn = true;
}

void UserSession::destroySession()
{
    m_userId = 0;
    m_username = "";
    m_isLoggedIn = false;
}

int UserSession::getUserId() const
{
    return m_userId;
}

QString UserSession::getUsername() const
{
    return m_username;
}

bool UserSession::isLoggedIn() const
{
    return m_isLoggedIn;
}
