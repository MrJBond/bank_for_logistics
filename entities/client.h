#ifndef CLIENT_H
#define CLIENT_H

#include "entity.h"

class Client : public Entity
{
private:
    std::string m_name = "";
    std::string m_address = "";
    std::string m_bossName = "";
    std::string m_bossPhone = "";
    std::string m_accountantName = "";
    std::string m_accountantPhone = "";
public:
    Client() = default;
    Client(const int id, const std::string& address, const std::string& name,
           const std::string& bossName, const std::string& bossPhone,
        const std::string& accountantName, const std::string& accountantPhone);
    ~Client() = default;

    std::string getName() const;
    void setName(const std::string s);
    std::string getAddress() const;
    void setAddress(const std::string s);
    std::string getBossName() const;
    void setBossName(const std::string s);
    std::string getBossPhone() const;
    void setBossPhone(const std::string s);
    std::string getAccountantName() const;
    void setAccountantName(const std::string s);
    std::string getAccountantPhone() const;
    void setAccountantPhone(const std::string s);
};
inline QDebug operator<<(QDebug os, const Client& c){
    os << "Client: " << c.getId() << " " << c.getName() << " "
       << c.getAddress() << " " << c.getBossName() << " "
       << c.getBossPhone() << " " << " " << c.getAccountantName()
       << " " << c.getAccountantPhone();
    return os;
}
inline void operator<<(QTextBrowser* browser, const Client& c){
    QString res;
    res += QString::number(c.getId()) + "   ";
    res += c.getName() + "   ";
    res += c.getAddress() + "   ";
    res += c.getBossName() + "   ";
    res += c.getBossPhone() + "   ";
    res += c.getAccountantName() + "   ";
    res += c.getAccountantPhone() + '\n';
    browser->append(res);
}
#endif // CLIENT_H
