#include "accountrepository.h"
#include "entities/account.h"

AccountRepository::AccountRepository() {}
AccountRepository::~AccountRepository() {}

std::vector<std::shared_ptr<Entity>> AccountRepository::getAll() const {
    std::vector<std::shared_ptr<Entity>> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM \"Account\"")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        int id = query.value(0).toInt();
        double amount = query.value(1).toDouble();
        QString currency = query.value(2).toString();
        int id_client = query.value(3).toInt();
        try{
           auto account = std::make_shared<Account>(id, id_client, amount, currency);
           res.push_back(account);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}
void AccountRepository::insert(std::shared_ptr<Entity> entity) {
    if(entity == nullptr){
        return;
    }
    if(Account* acc = dynamic_cast<Account*>(entity.get()); acc != nullptr){
        QSqlQuery query;
        query.prepare(QString("INSERT INTO public.\"Account\" ") +
                      "(amount, currency, id_client)" +
                      "VALUES(:amount, :currency, :id_client);");
        query.bindValue(":amount", acc->getAmount());
        query.bindValue(":currency", acc->getCurrency());
        query.bindValue(":id_client", acc->getClientId());

        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
        // Get the last inserted ID
        QVariant insertedId = query.lastInsertId();
        if (insertedId.isValid()) {
            int id = insertedId.toInt();
            entity->setId(id);
        }
    }
}
void AccountRepository::update(std::shared_ptr<Entity> entity){
    if(entity == nullptr){
        return;
    }
    if(Account* acc = dynamic_cast<Account*>(entity.get()); acc != nullptr){
        QSqlQuery query;
        query.prepare("UPDATE public.\"Account\" "
                      "SET amount = :amount, "
                      "currency = :currency, "
                      "id_client = :id_client "
                      "WHERE id_account = :id"
                      );
        query.bindValue(":amount", acc->getAmount());
        query.bindValue(":currency", acc->getCurrency());
        query.bindValue(":id_client", acc->getClientId());
        query.bindValue(":id", acc->getId());

        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
    }
}
void AccountRepository::remove(int id){
    AbstractRepository::remove("id_account", "Account", id);
}
std::vector<AccountRepository::clientLoanAccount> AccountRepository::getClientLoanAccountView() const{
    std::vector<clientLoanAccount> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM client_loan_account_view")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        clientLoanAccount c;
        c.id_account = query.value(0).toInt();
        c.id_client = query.value(1).toInt();
        c.id_loan = query.value(2).toInt();
        c.account_amount = query.value(3).toDouble();
        c.loan_amount = query.value(4).toDouble();
        res.push_back(c);
    }
    return res;
}
// using SQL transaction
void AccountRepository::updateAmountOnTransactions(){
    /*account.amount -= Transaction.amount and
     accountTo.amount += Transaction.amount*/
    QSqlQuery query;

    // Start the transaction
    if (!query.exec("BEGIN TRANSACTION;")) {
        qDebug() << "Failed to start transaction:" << query.lastError().text();
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    // First UPDATE statement
    QString updateOutgoing = R"(
    UPDATE public."Account" AS a
    SET amount = a.amount - t.total_amount
    FROM (
        SELECT id_account, SUM(amount) AS total_amount
        FROM public."Transaction"
        GROUP BY id_account
    ) AS t
    WHERE a.id_account = t.id_account;
)";
    if (!query.exec(updateOutgoing)) {
        qDebug() << "Failed to execute outgoing update:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    // Second UPDATE statement
    QString updateIncoming = R"(
    UPDATE public."Account" AS a
    SET amount = a.amount + t.total_amount
    FROM (
        SELECT "id_accountTo", SUM(amount) AS total_amount
        FROM public."Transaction"
        GROUP BY "id_accountTo"
    ) AS t
    WHERE a.id_account = t."id_accountTo";
)";
    if (!query.exec(updateIncoming)) {
        qDebug() << "Failed to execute incoming update:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    // Commit the transaction
    if (!query.exec("COMMIT TRANSACTION;")) {
        qDebug() << "Failed to commit transaction:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    qDebug() << "Transaction completed successfully.";
}
void AccountRepository::addLoanToAccount(int id_account, double amount){
    // account.amount += n and INSERT INTO Loan
    QSqlQuery query;

    // Start the transaction
    if (!query.exec("BEGIN TRANSACTION;")) {
        qDebug() << "Failed to start transaction:" << query.lastError().text();
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    QString updateAccount = R"(UPDATE public."Account"
                          SET amount = amount + :amount WHERE id_account = :id_account;)";
    query.prepare(updateAccount);
    query.bindValue(":amount", amount);
    query.bindValue(":id_account", id_account);
    if (!query.exec()) {
        qDebug() << "Failed to execute the update:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    QString insertLoan = R"(INSERT INTO "Loan" (id_account, amount)
                            VALUES(:id_account,:amount);)";
    query.prepare(insertLoan);
    query.bindValue(":amount", amount);
    query.bindValue(":id_account", id_account);
    if (!query.exec()) {
        qDebug() << "Failed to execute the insert:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    // Commit the transaction
    if (!query.exec("COMMIT TRANSACTION;")) {
        qDebug() << "Failed to commit transaction:" << query.lastError().text();
        query.exec("ROLLBACK;");
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    qDebug() << "Transaction completed successfully.";
}
