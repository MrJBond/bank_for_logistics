#include "accountservice.h"

AccountService::AccountService() {
    m_client_repo = std::make_shared<ClientRepository>(ClientRepository());
}
void AccountService::getAllAccounts(QTextBrowser* browser, QTableWidget *table)const{
    std::vector<std::shared_ptr<Entity>> res = m_account_repo->getAll();
    if(table != nullptr){
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"id", "Amount", "Currency", "Client Id"});
        table->setRowCount(res.size());
    }
    if(browser != nullptr)
        browser->append("id   Amount   Currency   Client Id\n");
    for(size_t i = 0;i < res.size(); ++i){
        const auto ent = res[i];
        Account* account = dynamic_cast<Account*>(ent.get());
        if(account){
            qDebug() << *account;
            if(browser != nullptr)
                browser << *account;
            if(table != nullptr){
                table->setItem(i, 0, new QTableWidgetItem(QString::number(account->getId())));
                table->item(i,0)->setFlags(table->item(i,0)->flags() & ~Qt::ItemIsEditable);
                table->setItem(i, 1, new QTableWidgetItem(QString::number(account->getAmount())));
                table->setItem(i, 2, new QTableWidgetItem(account->getCurrency()));
                table->setItem(i, 3, new QTableWidgetItem(QString::number(account->getClientId())));
            }
        }
    }
}

void AccountService::insertAccount(int id_client, double amount, QString currency){
    std::shared_ptr<Account> acc = std::make_shared<Account>();
    bool isClient = isPresent<Client>(id_client, [&](){return m_client_repo->getAll();});
    bool ok = true;
    try{
        acc->setAmount(amount);
        acc->setCurrency(currency);
        acc->setClientId(id_client);
    }catch(const std::invalid_argument& e){
        ok = false;
        qDebug() << e.what();
    }
    if(ok && isClient)
        m_account_repo->insert(acc);
    else
        throw std::invalid_argument("The account is invalid!");
}

void AccountService::updateAccount(int id, int id_client, double amount,
                   QString currency){
    if(!isPresent<Client>(id_client, [&](){return m_client_repo->getAll();})){
        throw std::invalid_argument("Update: There is no such client!");
    }
    // this may throw
    auto account = std::make_shared<Account>(id, id_client, amount, currency);
    m_account_repo->update(account);
}
void AccountService::deleteAccount(int id){
    bool isPresentA = isPresent<Account>(id, [&](){return m_account_repo->getAll();});
    if(!isPresentA){
        throw std::invalid_argument("Delete: There is no such account!");
    }
    m_account_repo->remove(id);
}
void AccountService::getClientLoanAccountView(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_account_repo->getClientLoanAccountView();
    browser->append("id_account   id_client   id_loan  account_amount   loan_amount\n");
    for(const auto& ent : res){
        browser->append(QString::number(ent.id_account) + "   "
                        + QString::number(ent.id_client) + "   "
                        + QString::number(ent.id_loan) + "   "
                        + QString::number(ent.account_amount) + "   "
                        + QString::number(ent.loan_amount) + "\n");
    }
}
void AccountService::updateAmountOnTransactions(){
    // this may throw
    m_account_repo->updateAmountOnTransactions();
}
void AccountService::addLoanToAccount(int id_account, double amount){
    if(!isPresent<Account>(id_account, [&](){return m_account_repo->getAll();})){
       throw std::invalid_argument("AddLoanToAccount: There is no such account!");
    }
    if(amount  < 0){
        throw std::invalid_argument("AddLoanToAccount: There amount is invalid!");
    }
    // this may throw
    m_account_repo->addLoanToAccount(id_account, amount);
}
