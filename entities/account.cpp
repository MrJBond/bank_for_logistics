#include "account.h"

Account::Account(const int id, const int id_client, const double amount, const QString& currency)
    :Entity(id), m_amount(amount){
    setClientId(id_client);
    setCurrency(currency);
}
Account::~Account() {}

QString Account::getCurrency() const{
    return m_currency;
}
int Account::getClientId() const{
    return m_id_client;
}
double Account::getAmount() const{
    return m_amount;
}
void Account::setCurrency(const QString currency){
    const bool correct = isCurrencySupported(currency);
    if(correct){
        m_currency = currency;
    }else{
        throw std::invalid_argument("The currency is invalid");
    }
}
void Account::setClientId(const int id_client){
    if(id_client <= 0){
        throw std::invalid_argument("The client's id is invalid");
    }else{
        m_id_client = id_client;
    }
}
void Account::setAmount(const double amount){
    m_amount = amount; // may be negative
}
