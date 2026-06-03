#ifndef TESTER_H
#define TESTER_H

#include <QObject>
#include "AI/chatbot.h"
#include "AI/networkmanager.h"
#include "entities/transaction.h"

class Tester : public NetworkManager
{
    Q_OBJECT
private:
    // Chat bot tests
    struct ChatTestCase {
        QString message;
        QString expectedIntent;
    };
    int m_chatTestsCompleted = 0;
    int m_chatTestsPassed = 0;
    int m_chatTestsTotal = 0;

    // Transaction categorization
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
    void testChatbotIntents();
signals:
    void transactionsReady(const std::vector<TestTransaction>& trans);
private slots:
    void handleNetworkError(const QString& errorString);
    void onTransactionsReady(const std::vector<TestTransaction>& trans);
};

#endif // TESTER_H
