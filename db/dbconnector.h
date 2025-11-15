#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include <QtSql>
#include <QSqlError>
#include <QDebug>
#include <QSqlQuery>

class DbConnector {
public:
    static DbConnector* getInstance();
    ~DbConnector() = default;
    DbConnector(DbConnector& other) = delete;
    void operator=(const DbConnector& other) = delete;
    void reConnect(const QString& name, const QString& password);
private:
    DbConnector();
    QSqlDatabase db;
};

#endif // DBCONNECTOR_H
