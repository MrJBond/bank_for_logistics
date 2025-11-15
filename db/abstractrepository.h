#ifndef ABSTRACTREPOSITORY_H
#define ABSTRACTREPOSITORY_H
#include "entities/entity.h"
#include <vector>
#include <memory>
#include <QSqlQuery>
#include <QSqlError>

class AbstractRepository
{
protected:
    void remove(QString id_name, QString table_name, int id);
public:
    AbstractRepository();
    virtual ~AbstractRepository();
    virtual std::vector<std::shared_ptr<Entity>> getAll() const = 0;
    virtual void insert(std::shared_ptr<Entity> entity) = 0;
    virtual void update(std::shared_ptr<Entity> entity) = 0;
    virtual void remove(int id) = 0;
};

#endif // ABSTRACTREPOSITORY_H
