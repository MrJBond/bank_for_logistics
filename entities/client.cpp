#include "client.h"

Client::Client(int id, std::string address, std::string name,
               std::string bossName, std::string bossPhone,
               std::string accountantName, std::string accountantPhone):
    Entity(id),
    m_address(address), m_bossName(bossName), m_bossPhone(bossPhone),
    m_accountantName(accountantName), m_accountantPhone(accountantPhone){
    setName(name);
}

std::string Client::getName() const{
    return m_name;
}
void Client::setName(const std::string s){
    if(s == "")
        throw std::invalid_argument("The client's name must be non-empty and unique!");
    m_name = s;
}
std::string Client::getAddress() const{
    return m_address;
}
void Client::setAddress(const std::string s){
    m_address = s;
}
std::string Client::getBossName() const{
    return m_bossName;
}
void Client::setBossName(const std::string s){
    m_bossName = s;
}
std::string Client::getBossPhone() const{
    return m_bossPhone;
}
void Client::setBossPhone(const std::string s){
    m_bossPhone = s;
}
std::string Client::getAccountantName() const{
    return m_accountantName;
}
void Client::setAccountantName(const std::string s){
    m_accountantName = s;
}
std::string Client::getAccountantPhone() const{
    return m_accountantPhone;
}
void Client::setAccountantPhone(const std::string s){
    m_accountantPhone = s;
}
