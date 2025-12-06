#ifndef LOAN_H
#define LOAN_H

#include "entity.h"

class Loan : public Entity
{
private:
    int m_id_account = 0;
    QDate m_issue_date;
    QDate m_usage_date;
    double m_percent = 0.;
    double m_amount = 0.;

    // Helper method to validate the date constraint
    inline void validateDates()
    {
        // Only validate if both dates are set
        if (m_issue_date.isValid() && m_usage_date.isValid())
        {
            if (m_issue_date > m_usage_date)
            {
                throw std::invalid_argument("Issue date must be less than or equal to the usage date");
            }
        }
    }
public:
    Loan(const int id, const int id_account, const QDate& issue_date, const QDate& usage_date, const double percent, const double amount);
    Loan() = default;
    ~Loan() = default;

    int getId_account() const;
    QDate getIssue_date() const;
    QDate getUsage_date() const;
    double getPercent() const;
    double getAmount() const;

    void setId_account(const int id);
    void setIssue_date(const QDate date);
    void setUsage_date(const QDate date);
    void setPercent(const double percent);
    void setAmount(const double amount);
};
inline QDebug operator<<(QDebug os, const Loan& l){
    os << "Loan: " << l.getId() << " " << l.getId_account() << " "
       << l.getIssue_date() << " " << l.getUsage_date() << " "
       << l.getPercent() << " " << l.getAmount();
    return os;
}
inline void operator<<(QTextBrowser* browser, const Loan& l){
    QString res;
    res += QString::number(l.getId()) + "   ";
    res += QString::number(l.getId_account()) + "   ";
    res += l.getIssue_date().toString() + "   ";
    res += l.getUsage_date().toString() + "   ";
    res += QString::number(l.getPercent()) + "   ";
    res += QString::number(l.getAmount()) + '\n';
    browser->append(res);
}
#endif // LOAN_H
