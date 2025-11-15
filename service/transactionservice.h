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
    void getAllTransactions(QTextBrowser* browser, QTableWidget *table = nullptr) const;
    void insertTransaction(QDate date, double amount,
                           int id_account, int id_accountTo);
    void updateTransaction(int id, QDate date, double amount,
                           int id_account, int id_accountTo);
    void deleteTransaction(int id);
    void getTransactionView(QTextBrowser* browser) const;
    // charts
    void buildTransactionsChart(const int w, const int h) const;
};

#endif // TRANSACTIONSERVICE_H
