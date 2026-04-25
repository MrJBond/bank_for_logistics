#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "entity.h"

class Transaction : public Entity
{
private:
    QDate m_date;
    double m_amount = 0.;
    int m_id_account = 0;
    int m_id_accountTo = 0;
    QString m_description = "";
    QString m_location = "";
public:
    Transaction() = default;
    Transaction(const int id, const QDate& date, const double amount, const int id_account, const int id_accountTo, const QString& description, const QString& location);
    ~Transaction() = default;
    double getAmount() const;
    QDate getDate() const;
    int getIdAccount() const;
    int getIdAccountTo() const;
    QString getDescription() const;
    QString getLocation() const;

    void setAmount(const double amount);
    void setDate(const QDate date);
    void setIdAccount(const int id);
    void setIdAccountTo(const int id);
    void setDescription(const QString& description);
    void setLocation(const QString& location);
};
inline QDebug operator<<(QDebug os, const Transaction& t){
    os << "Transaction: " << t.getId() << " " << t.getAmount() << " "
       << t.getDate() << " " << t.getIdAccount() << " "
       << t.getIdAccountTo() << " " << t.getDescription() << " " << t.getLocation();
    return os;
}
void operator<<(QTextBrowser* browser, const Transaction& t);
#endif // TRANSACTION_H
