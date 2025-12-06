#include "loan.h"

Loan::Loan(const int id, const int id_account, const QDate& issue_date, const QDate& usage_date, const double percent, const double amount):
    Entity(id)
{
    setId_account(id_account);
    setIssue_date(issue_date);
    setUsage_date(usage_date);
    setPercent(percent);
    setAmount(amount);
}

int Loan::getId_account() const{
    return m_id_account;
}
QDate Loan::getIssue_date() const{
    return m_issue_date;
}
QDate Loan::getUsage_date() const{
    return m_usage_date;
}
double Loan::getPercent() const{
    return m_percent;
}
double Loan::getAmount() const{
    return m_amount;
}

void Loan::setId_account(const int id){
    if(id <= 0){
        throw std::invalid_argument("The account id is invalid!");
    }
    m_id_account = id;
}

void Loan::setIssue_date(const QDate date){
    if(date.isNull()){
        std::chrono::year_month_day today = // default
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        m_issue_date = QDate(int(today.year()), unsigned(today.month()), unsigned(today.day()));
    }else{
        m_issue_date = date;
    }
    validateDates();
}
void Loan::setUsage_date(const QDate date){
    if(date.isNull()){
        std::chrono::year_month_day today = // default
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        m_usage_date = QDate(int(today.year()), unsigned(today.month()), unsigned(today.day()));
    }else{
        m_usage_date = date;
    }
    validateDates();
}
void Loan::setPercent(const double percent){
    if(percent < 0 || percent > 100){
        throw std::invalid_argument("The percent is invalid!");
    }
    m_percent = percent;
}
void Loan::setAmount(const double amount){
    if(amount < 0){
        throw std::invalid_argument("The amount is invalid!");
    }
    m_amount = amount;
}
