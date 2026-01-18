#ifndef FRAUDDETECTOR_H
#define FRAUDDETECTOR_H

#include "AI/networkmanager.h"
#include "entities/transaction.h"

class FraudDetector : public NetworkManager
{
    Q_OBJECT
public:
    FraudDetector();
    ~FraudDetector();
    void requestTransactionCheck(const Transaction& t, const QJsonArray& history);
    void requestTransactionCategorization(const Transaction& t);
    void requestTransactionCategorization(const std::vector<Transaction>& trans);
signals:
    void transactionChecked(bool isSuspicious, const double score, const Transaction& t);
    void transactionCategorized(const QString& category, const QString& icon, const Transaction& t);
};

#endif // FRAUDDETECTOR_H
