#include "dbconnector.h"

DbConnector::DbConnector() {
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("Bank");
    db.setUserName("bank_app_user");
    db.setPassword("qwerty");
    db.setPort(5432);
    if (!db.open()) {
        qCritical() << "Database connection failed:" << db.lastError().text();
        throw std::runtime_error("Failed to connect to database");
    }
    qDebug() << "Database connected successfully!";
}

DbConnector* DbConnector::getInstance() {
    static DbConnector instance;  // Thread-safe singleton
    return &instance;
}
QSqlDatabase* DbConnector::getDb() {
    return &db;
}
void DbConnector::reConnect(const QString& name, const QString& password){
    QString curUserName = db.userName();
    QString curUserPassword = db.password();
    db.close();
    db.setUserName(name);
    db.setPassword(password);
    if (!db.open()) {
        // keep the current user
        db.setUserName(curUserName);
        db.setPassword(curUserPassword);
        if(!db.open()){
            throw std::runtime_error("Failed to keep the current user!");
        }
        qCritical() << "Database connection failed:" << db.lastError().text();
        throw std::runtime_error("Failed to connect to database");
    }
}
