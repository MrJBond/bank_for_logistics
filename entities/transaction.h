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
public:
    Transaction() = default;
    Transaction(int id, QDate date, double amount, int id_account, int id_accountTo);
    ~Transaction() = default;
    double getAmount() const;
    QDate getDate() const;
    int getIdAccount() const;
    int getIdAccountTo() const;

    void setAmount(const double amount);
    void setDate(const QDate date);
    void setIdAccount(const int id);
    void setIdAccountTo(const int id);
};
inline QDebug operator<<(QDebug os, const Transaction& t){
    os << "Transaction: " << t.getId() << " " << t.getAmount() << " "
       << t.getDate() << " " << t.getIdAccount() << " "
       << t.getIdAccountTo();
    return os;
}
inline void operator<<(QTextBrowser* browser, const Transaction& t){
    QString res;
    res += QString::number(t.getId()) + "   ";
    res += t.getDate().toString() + "   ";
    res += QString::number(t.getAmount()) + "   ";
    res += QString::number(t.getIdAccount()) + "   ";
    res += QString::number(t.getIdAccountTo()) + '\n';
    browser->append(res);
}
#endif // TRANSACTION_H
