#ifndef TRANSACTIONSERVICE_H
#define TRANSACTIONSERVICE_H

#include "db/transactionrepository.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"

class TransactionService : public AbstractService
{
private:
    std::shared_ptr<TransactionRepository> m_transaction_repo = nullptr;
public:
    TransactionService();
    ~TransactionService() = default;
    void getAll(QTextBrowser* browser, QTableWidget *table = nullptr) const override;
    int insertTransaction(QDate date, double amount,
                           int id_account, int id_accountTo);
    void updateTransaction(int id, QDate date, double amount,
                           int id_account, int id_accountTo);
    void deleteObj(const int id) override;
    void getTransactionView(QTextBrowser* browser) const;
    void makeTransaction(const int id_account, const int id_accountTo, const double amount);
    // charts
    void buildTransactionsChart(const int w, const int h) const;
};

#endif // TRANSACTIONSERVICE_H
