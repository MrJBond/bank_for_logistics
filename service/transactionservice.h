#ifndef TRANSACTIONSERVICE_H
#define TRANSACTIONSERVICE_H

#include "db/transactionrepository.h"
#include "db/clientrepository.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"
#include "AI/frauddetector.h"

class TransactionService : public AbstractService
{
private:
    std::shared_ptr<TransactionRepository> m_transaction_repo = nullptr;
    std::shared_ptr<FraudDetector> m_fraudDetector = nullptr;
    // fraud detection
    bool isTransactionSuspicious(const Transaction& transaction);
    void makeTransaction(const int id_account, const int id_accountTo, const double amount, const QString& description);
    // Define a struct to save the context
    struct PendingTx {
        int fromAccount;
        int toAccount;
        double amount;
        QString description;
    };
    // The storage variable
    std::optional<PendingTx> m_pendingTx;
public:
    TransactionService();
    ~TransactionService() = default;
    void getAll(QTextBrowser* browser, QTableWidget *table = nullptr) const override;
    int insertTransaction(const QDate& date, const double amount,
                           const int id_account, const int id_accountTo, const QString& description);
    void updateTransaction(const int id, const QDate& date, const double amount,
                           const int id_account, const int id_accountTo, const QString& description);
    void deleteObj(const int id) override;
    void getTransactionView(QTextBrowser* browser) const;
    void requestTransaction(const int id_account, const int id_accountTo, const double amount, const QString& description);
    // charts
    void buildTransactionsChart(const int w, const int h) const;
    void buildSpendingPieChart(const int w, const int h) const;
private slots:
    void handleNetworkFailure(const QString& errorString);
    void handleTransactionChecked(bool isSuspicious, const double score, const Transaction& t);
    void handleTransactionCategorized(const QString& category, const QString& icon, const Transaction& t);
    // Call this when Face ID returns success
    void handleUserVerification();

    // Call this if Face ID fails or is cancelled
    void cancelPendingTransaction();
};

#endif // TRANSACTIONSERVICE_H
