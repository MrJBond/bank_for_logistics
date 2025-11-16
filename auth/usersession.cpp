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

int UserSession::login(const QString& username, const QString& password)
{
    // 1. Hash the password the user typed
    // We use SHA-256 for this example
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

    // 2. Query the database
    QSqlQuery query;
    query.prepare("SELECT id_client, password_hash FROM \"ClientAuth\" WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec() || !query.next()) {
        QString message = "Login failed: User not found or DB error User name: " + username;
        throw std::runtime_error(message.toStdString());
    }

    // 3. Check the password hash
    QByteArray storedHash = query.value("password_hash").toByteArray();
    if (passwordHash == storedHash) {
        // 4. LOGIN SUCCESS!
        int loggedInClientId = query.value("id_client").toInt();
        qDebug() << "Login successful for client ID: " << loggedInClientId;
        return loggedInClientId;
    } else {
        QString message =  "Login failed: Invalid password User name: " + username;
        throw std::runtime_error(message.toStdString());
    }
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
