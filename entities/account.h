#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "entities/entity.h"

class Account : public Entity
{
private:
    int m_id_client = 0;
    double m_amount = 0.;
    QString m_currency = DOLLAR;
public:
    Account() = default;
    Account(const int id, const int id_client, const double amount, const QString& currency);
    ~Account();
    QString getCurrency() const;
    int getClientId() const;
    double getAmount() const;
    void setCurrency(const QString currency);
    void setClientId(const int id);
    void setAmount(const double amount);
};
inline QDebug operator<<(QDebug os, const Account& a){
    os << "Account: " << a.getId() << " " << a.getAmount() << " "
       << a.getCurrency() << " " << a.getClientId();
    return os;
}
inline void operator<<(QTextBrowser* browser, const Account& a){
    QString res;
    res += QString::number(a.getId()) + "   ";
    res += QString::number(a.getAmount()) + "   ";
    res += a.getCurrency() + "   ";
    res += QString::number(a.getClientId()) + '\n';
    browser->append(res);
}
#endif // ACCOUNT_H
