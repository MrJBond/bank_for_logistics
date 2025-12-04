#ifndef ACCOUNTSERVICE_H
#define ACCOUNTSERVICE_H

#include "db/accountrepository.h"
#include "db/clientrepository.h"
#include "entities/account.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"

class AccountService : public AbstractService
{
private:
    std::shared_ptr<ClientRepository> m_client_repo = nullptr; // Account should know about clients
public:
    AccountService();
    ~AccountService() = default;
    void getAllAccounts(QTextBrowser* browser, QTableWidget *table = nullptr) const;
    int insertAccount(int id_client, double amount,
                       QString currency);
    void updateAccount(int id, int id_client, double amount,
                       QString currency);
    void deleteAccount(int id);
    void getClientLoanAccountView(QTextBrowser* browser) const;
    void updateAmountOnTransactions();
    void addLoanToAccount(int id_account, double amount);
    void currencyChart(const int w, const int h) const;
};

#endif // ACCOUNTSERVICE_H
