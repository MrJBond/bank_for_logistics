#include "abstractrepository.h"

AbstractRepository::AbstractRepository() {}

AbstractRepository::~AbstractRepository() {}

void AbstractRepository::remove(QString id_name, QString table_name, int id) {
    QSqlQuery query;
    QString queryString = QString("DELETE FROM \"%1\" WHERE %2 = :id")
                              .arg(table_name)
                              .arg(id_name);
    query.prepare(queryString);
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
}
