#include "transactionservice.h"
#include "entities/transaction.h"

TransactionService::TransactionService() {
    m_transaction_repo = std::make_shared<TransactionRepository>(TransactionRepository());
}
void TransactionService::getAll(QTextBrowser* browser, QTableWidget *table)const{
    std::vector<std::shared_ptr<Entity>> res = m_transaction_repo->getAll();
    if(table != nullptr){
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({"id", "Date", "Amount", "Id account", "Id account To"});
        table->setRowCount(res.size());
    }
    if(browser != nullptr)
        browser->append("id   Date   Amount   Id account   Id account To\n");
    for(size_t i = 0;i < res.size(); ++i){
        const auto ent = res[i];
        Transaction* transact = dynamic_cast<Transaction*>(ent.get());
        if(transact){
            qDebug() << *transact;
            if(browser != nullptr)
                browser << *transact;
            if(table != nullptr){
                table->setItem(i, 0, new QTableWidgetItem(QString::number(transact->getId())));
                table->item(i,0)->setFlags(table->item(i,0)->flags() & ~Qt::ItemIsEditable);
                table->setItem(i, 1, new QTableWidgetItem(transact->getDate().toString()));
                table->setItem(i, 2, new QTableWidgetItem(QString::number(transact->getAmount())));
                table->setItem(i, 3, new QTableWidgetItem(QString::number(transact->getIdAccount())));
                table->setItem(i, 4, new QTableWidgetItem(QString::number(transact->getIdAccountTo())));
            }
        }
    }
}
int TransactionService::insertTransaction(QDate date, double amount,
                                           int id_account, int id_accountTo){
    std::shared_ptr<Transaction> tran = std::make_shared<Transaction>();
    std::vector<std::shared_ptr<Entity>> accounts = m_account_repo->getAll();

    // check id
    bool isAccount = isPresent(id_account, m_account_repo.get());
    bool isAccountTo = isPresent(id_accountTo, m_account_repo.get());
    bool ok = true;
    try{
        tran->setAmount(amount);
        tran->setDate(date);
        tran->setIdAccount(id_account);
        tran->setIdAccountTo(id_accountTo);
    }catch(const std::invalid_argument& e){
        ok = false;
        qDebug() << e.what();
    }
    if(ok && isAccount && isAccountTo){
        m_transaction_repo->insert(tran);
        return tran->getId();
    }
    else
        throw std::invalid_argument("The transaction is invalid!");
}
void TransactionService::updateTransaction(int id, QDate date, double amount,
                       int id_account, int id_accountTo){
    // check id
    if(!isPresent(id_account, m_account_repo.get())){
        throw std::invalid_argument("Update: There is no source account!");
    }
    if(!isPresent(id_accountTo, m_account_repo.get())){
        throw std::invalid_argument("Update: There is no destination account!");
    }
    // this may throw
    auto tran = std::make_shared<Transaction>(id, date, amount, id_account, id_accountTo);
    m_transaction_repo->update(tran);
}
void TransactionService::deleteObj(const int id){
    deleteHelper(id, m_transaction_repo.get());
}
void TransactionService::getTransactionView(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    browser->append("id   Date   Amount   Id account   Id account To\n");
    std::vector<Transaction> res = m_transaction_repo->transactionView();
    for(const Transaction& t : res){
        browser << t;
    }
}
void TransactionService::buildTransactionsChart(const int w, const int h) const{
    std::vector<std::shared_ptr<Entity>> res = m_transaction_repo->getAll();
    QStringList categories;
    QBarSet* barSet = new QBarSet("Amount");
    std::map<int, double> amountSums;
    for(const auto& ent : res){
        Transaction* transaction = dynamic_cast<Transaction*>(ent.get());
        if(transaction)
            amountSums[transaction->getIdAccount()] += transaction->getAmount();
    }
    for(const auto& t : amountSums){
        // Only append the amount to the bar height
        *barSet << t.second;
        // Save the ID to use as the label below the bar later
        categories << QString::number(t.first);
    }
    auto toolTipText = [&](const int index) -> QString {
        // Retrieve the value of the bar using the index
        double amount = barSet->at(index);
        // Format the text
        QString toolTipText = QString("Amount: %1 ").arg(amount, 0, 'f', 2);
        return toolTipText;
    };
    QChartView* barChart = createBarChart(barSet, "Account Transactions", categories, toolTipText);
    createChartBox(barChart, w, h);
}

void TransactionService::makeTransaction(const int id_account, const int id_accountTo, const double amount){

}
