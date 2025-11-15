#ifndef TRANSACTIONREPOSITORY_H
#define TRANSACTIONREPOSITORY_H

#include "db/abstractrepository.h"
#include "entities/transaction.h"

class TransactionRepository : public AbstractRepository
{
public:
    TransactionRepository();
    ~TransactionRepository();
    std::vector<std::shared_ptr<Entity>> getAll() const override;
    void insert(std::shared_ptr<Entity> entity) override;
    void update(std::shared_ptr<Entity> entity) override;
    void remove(int id) override;
    std::vector<Transaction> transactionView() const;
};

#endif // TRANSACTIONREPOSITORY_H
