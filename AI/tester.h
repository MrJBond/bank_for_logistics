#ifndef TESTER_H
#define TESTER_H

#include <QObject>
#include "AI/networkmanager.h"
#include "entities/transaction.h"

class Tester : public NetworkManager
{
    Q_OBJECT
private:
    struct TestCase {
        QString description;
        QString expectedCategory;
        QString expectedIcon;
    };
    struct TestTransaction{
        int id;
        QString category;
        QString icon;
    };
    std::vector<TestTransaction> m_expectedTransactionResult;
    void requestTransactionCategorization(const std::vector<Transaction>& trans);
public:
    Tester();
    void testTransactionCategorization();
signals:
    void transactionsReady(const std::vector<TestTransaction>& trans);
private slots:
    void handleNetworkError(const QString& errorString);
    void onTransactionsReady(const std::vector<TestTransaction>& trans);
};

#endif // TESTER_H
