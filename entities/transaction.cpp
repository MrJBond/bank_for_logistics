#include "transaction.h"

Transaction::Transaction(const int id, const QDate& date, const double amount, const int id_account, const int id_accountTo):
    Entity(id)
{
    setDate(date);
    setAmount(amount);
    setIdAccount(id_account);
    setIdAccountTo(id_accountTo);
}

double Transaction::getAmount() const{
    return m_amount;
}
QDate Transaction::getDate() const{
    return m_date;
}
int Transaction::getIdAccount() const{
    return m_id_account;
}
int Transaction::getIdAccountTo() const{
    return m_id_accountTo;
}

void Transaction::setAmount(const double amount){
    if(amount < 0){
        throw std::invalid_argument("The amount of money on the account is invalid!");
    }
    m_amount = amount;
}
void Transaction::setDate(const QDate date){
    if(date < QDate(2024, 1,1)){
        throw std::invalid_argument("The date must be >= 2024-01-01");
    }
    if(date.isNull()){
        std::chrono::year_month_day today = // default
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        m_date = QDate(int(today.year()), unsigned(today.month()), unsigned(today.day()));
    }else{
        m_date = date;
    }
}

void Transaction::setIdAccount(const int id){
    if(id <= 0){
       throw std::invalid_argument("The account id is invalid!");
    }
    m_id_account = id;
}
void Transaction::setIdAccountTo(const int id){
    if(id <= 0){
        throw std::invalid_argument("The destination account id is invalid!");
    }
    m_id_accountTo = id;
}
