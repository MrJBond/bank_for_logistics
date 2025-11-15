#ifndef ACCOUNTREPOSITORY_H
#define ACCOUNTREPOSITORY_H

#include "abstractrepository.h"


class AccountRepository : public AbstractRepository
{
private:
    // for the view
    struct clientLoanAccount{int id_account = 0; int id_client = 0;
        int id_loan = 0; double account_amount = 0.; double loan_amount = 0.;};
public:
    AccountRepository();
    ~AccountRepository();
    std::vector<std::shared_ptr<Entity>> getAll() const override;
    void insert(std::shared_ptr<Entity> entity) override;
    void update(std::shared_ptr<Entity> entity) override;
    void remove(int id) override;
    std::vector<clientLoanAccount> getClientLoanAccountView() const;
    // using the SQL transaction
    void updateAmountOnTransactions();
    // using the SQL transaction
    void addLoanToAccount(int id_account, double amount);
};

#endif // ACCOUNTREPOSITORY_H
