#include "transactionservice.h"
#include "entities/transaction.h"

TransactionService::TransactionService() {
    m_transaction_repo = std::make_shared<TransactionRepository>(TransactionRepository());
    m_fraudDetector = std::make_shared<FraudDetector>();
    connect(m_fraudDetector.get(), &FraudDetector::networkError,
            this, &TransactionService::handleNetworkFailure);
    connect(m_fraudDetector.get(), &FraudDetector::transactionChecked,
            this, &TransactionService::handleTransactionChecked);
    connect(m_session, &UserSession::userVerifiedSuccessfully,
            this, &TransactionService::handleUserVerification);
    connect(m_session, &UserSession::verificationFailed,
            this, &TransactionService::cancelPendingTransaction);
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

/***********************************************************
 *                      MAKE A TRANSACTION
 ************************************************************/
void TransactionService::requestTransaction(const int id_account, const int id_accountTo, const double amount){
    Transaction tempTran(INT_MAX, QDate::currentDate(), amount, id_account, id_accountTo);
    const bool suspicious = isTransactionSuspicious(tempTran);
}
void TransactionService::makeTransaction(const int id_account, const int id_accountTo, const double amount) {
    const int id = m_session->getUserId();
    if(id <= 0){
        const QString message = "User's id is invalid! id = " + QString::number(id);
        throw std::runtime_error(message.toStdString());
    }
    if(id_account == 0 || id_accountTo == 0){
        throw std::runtime_error("There is no account with id = 0");
    }
    if(id_account == id_accountTo){
        throw std::runtime_error("It doesn't make sense to send money to the same account!");
    }
    bool isAccount = isPresent(id_account, m_account_repo.get());
    bool isAccountTo = isPresent(id_accountTo, m_account_repo.get());
    if(!isAccount || !isAccountTo)
        throw std::runtime_error("The accounts aren't present!");
    if(!ClientRepository::isAccountMine(id, id_account)){
        throw std::runtime_error("The account isn't yours!");
    }
    QSqlDatabase *db = DbConnector::getInstance()->getDb();
    if(!db)
        throw std::runtime_error("The db is null");
    // 1. START TRANSACTION
    if (!db->transaction())
        throw std::runtime_error("Failed to start database transaction: " + db->lastError().text().toStdString());
    try {
        // 2. Validation: Amounts must be positive
        if (amount <= 0) {
            throw std::invalid_argument("Transaction amount must be positive.");
        }

        // 3. Fetch Accounts
        std::shared_ptr<Account> aFrom = std::dynamic_pointer_cast<Account>(m_account_repo->getById(id_account));
        std::shared_ptr<Account> aTo = std::dynamic_pointer_cast<Account>(m_account_repo->getById(id_accountTo));
        if (!aFrom || !aTo) {
            throw std::invalid_argument("Sender or Receiver account not found.");
        }

        // 4. Check Balance
        if (aFrom->getAmount() < amount) {
            throw std::runtime_error("Insufficient funds in sender account.");
        }

        // 5. Calculate Conversion
        // Logic: Convert Sender Currency -> USD -> Receiver Currency
        // Value in USD
        const double amountInUSD = Entity::toDollar(amount, aFrom->getCurrency());
        // Value in Target Currency
        const double amountFinal = Entity::fromDollar(amountInUSD, aTo->getCurrency());

        // 6. Update Balances
        // Subtract from Sender
        aFrom->setAmount(aFrom->getAmount() - amount);
        // Add to Receiver
        aTo->setAmount(aTo->getAmount() + amountFinal);

        // 7. Push Updates to DB
        m_account_repo->update(aFrom);
        m_account_repo->update(aTo);

        // 8. Create History Record
        // We do this LAST to ensure the math worked first
        insertTransaction(QDate::currentDate(), amount, id_account, id_accountTo);

        // 9. COMMIT TRANSACTION
        if (!db->commit()) {
            throw std::runtime_error("Failed to commit transaction: " + db->lastError().text().toStdString());
        }

    } catch (const std::exception& e) {
        // 10. ROLLBACK ON FAILURE
        // If anything above failed (insufficient funds, db error), undo everything.
        db->rollback();
        qCritical() << "Transaction failed, rolling back:" << e.what();
        throw; // Re-throw to let the UI know
    }
}
// fraud detection
bool TransactionService::isTransactionSuspicious(const Transaction& transaction){
    const int accountId = transaction.getIdAccount();
    QJsonArray historyArray;
    std::vector<Transaction> trans;
    try{
        trans = m_transaction_repo->getTransactionsForAccount(accountId);
    }catch(const std::exception& e){
        qDebug() << e.what();
        return false;
    }
    // Take the last 20 transactions, made from this account
    const int num = 20;
    const auto acc = dynamic_pointer_cast<Account>(m_account_repo->getById(accountId));
    std::sort(trans.begin(), trans.end(), [](const Transaction& t1, const Transaction& t2){
        return t1.getDate() > t2.getDate();
    });
    for(const Transaction& t : trans){
        if(historyArray.size() == num) break;
        if(t.getIdAccount() == accountId)
            historyArray.append(Entity::toDollar(t.getAmount(), acc->getCurrency()));
    }
    qDebug() << "History: " << historyArray;
    m_fraudDetector->requestTransactionCheck(transaction, historyArray);
    return true;
}
void TransactionService::handleNetworkFailure(const QString& errorString){
    qDebug() << "TransactionService: " << errorString;
}
void TransactionService::handleTransactionChecked(bool isSuspicious, const double score,  const Transaction& t){
    qDebug() << score << " " << isSuspicious;
    if(!isSuspicious){
        try {
            makeTransaction(t.getIdAccount(), t.getIdAccountTo(), t.getAmount());
            emit createMessageBox("Transaction successful!");
        } catch (const std::exception& e) {
            emit createMessageBox(e.what());
        }
        catch(...){
            const char* m = "Transaction failed";
            emit createMessageBox(m);
            qDebug() << m;
        }
    }else{
        // --- SUSPICIOUS PATH (Wait for Face ID) ---
        qDebug() << "Transaction paused for verification.";
        emit createMessageBox("Security Alert: This transaction is unusual. Face Verification required.");

        // 1. SAVE THE STATE
        // We copy the raw data into our member variable
        m_pendingTx = PendingTx{ t.getIdAccount(), t.getIdAccountTo(), t.getAmount() };

        // 2. TRIGGER VERIFICATION
        if (m_session->isLoggedIn()) {
            m_session->requestUserVerification();
        } else {
            emit createMessageBox("You are logged out!");
        }
    }
}
void TransactionService::handleUserVerification(){
    // success
    // 1. Check if we actually have something waiting
    if (!m_pendingTx.has_value()) {
        qWarning() << "Verification succeeded, but no transaction was pending.";
        return;
    }

    // 2. RETRIEVE THE STATE
    PendingTx tx = m_pendingTx.value();

    qDebug() << "Face ID Verified. Resuming transaction...";

    // 3. EXECUTE
    try {
        makeTransaction(tx.fromAccount, tx.toAccount, tx.amount);
        emit createMessageBox("Identity Verified. Transaction executed successfully.");
    } catch (const std::exception& e) {
        emit createMessageBox((QString("Transaction failed after verification: ") + e.what()).toStdString().c_str());
    }

    // 4. CLEANUP
    // Clear the memory so we don't accidentally run it again
    m_pendingTx.reset();
}
void TransactionService::cancelPendingTransaction() {
    if (m_pendingTx.has_value()) {
        qDebug() << "Pending transaction cancelled.";
        m_pendingTx.reset();
        emit createMessageBox("The Identity has NOT been Verified! Transaction cancelled!");
    }
}
