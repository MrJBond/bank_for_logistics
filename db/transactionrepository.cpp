#include "transactionrepository.h"
#include "entities/transaction.h"

TransactionRepository::TransactionRepository(){}

TransactionRepository::~TransactionRepository(){}

std::vector<std::shared_ptr<Entity>> TransactionRepository::getAll() const {
    std::vector<std::shared_ptr<Entity>> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM \"Transaction\" ORDER BY id_transaction")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        const int id = query.value(0).toInt();
        const QDate date = query.value(1).toDate();
        const double amount = query.value(2).toDouble();
        const int id_account = query.value(3).toInt();
        const int id_accountTo = query.value(4).toInt();
        const QString description = query.value(5).toString();
        // skip the category and the icon
        const QString location = query.value(8).toString();
        try{
            auto transact = std::make_shared<Transaction>(id, date, amount, id_account, id_accountTo, description, location);
            res.push_back(transact);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}

std::shared_ptr<Entity> TransactionRepository::getById(const int id) const{
    QSqlQuery query;
    query.prepare("SELECT * FROM \"Transaction\" WHERE id_transaction = :id");
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    if(query.next()){
        const int id = query.value(0).toInt();
        const QDate date = query.value(1).toDate();
        const double amount = query.value(2).toDouble();
        const int id_account = query.value(3).toInt();
        const int id_accountTo = query.value(4).toInt();
        const QString description = query.value(5).toString();
        // skip the category and the icon
        const QString location = query.value(8).toString();
        try{
            auto transact = std::make_shared<Transaction>(id, date, amount, id_account, id_accountTo, description, location);
            return transact;
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return nullptr; // not found
}

void TransactionRepository::insert(std::shared_ptr<Entity> entity) {
    if(entity == nullptr){
        return;
    }
    if(Transaction* tran = dynamic_cast<Transaction*>(entity.get()); tran != nullptr){
        // If description is empty, auto-generate one
        if (tran->getDescription().isEmpty()) {
            const QString desc = "Transfer to Account " + QString::number(tran->getIdAccountTo());
            tran->setDescription(desc);
        }
        QSqlQuery query;
        query.prepare(QString("INSERT INTO public.\"Transaction\" ") +
                      "(date_transaction, amount, id_account, \"id_accountTo\", description, location)" +
                      "VALUES(:date_transaction, :amount, :id_account, :id_accountTo, :desc, :location);");
        query.bindValue(":date_transaction", tran->getDate());
        query.bindValue(":amount", tran->getAmount());
        query.bindValue(":id_account", tran->getIdAccount());
        query.bindValue(":id_accountTo", tran->getIdAccountTo());
        query.bindValue(":desc", tran->getDescription());
        query.bindValue(":location", tran->getLocation());
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
void TransactionRepository::update(std::shared_ptr<Entity> entity){
    if(entity == nullptr){
        return;
    }
    if(Transaction* tran = dynamic_cast<Transaction*>(entity.get()); tran != nullptr){
        QSqlQuery query;
        query.prepare("UPDATE public.\"Transaction\" "
                      "SET date_transaction = :date_transaction, "
                      "amount = :amount, "
                      "id_account = :id_account, "
                      "\"id_accountTo\" = :id_accountTo, "
                      "description = :description, "
                      "location = :location "
                      "WHERE id_transaction = :id"
                      );
        query.bindValue(":date_transaction", tran->getDate());
        query.bindValue(":amount", tran->getAmount());
        query.bindValue(":id_account", tran->getIdAccount());
        query.bindValue(":id_accountTo", tran->getIdAccountTo());
        query.bindValue(":description", tran->getDescription());
        query.bindValue(":location", tran->getLocation());
        query.bindValue(":id", tran->getId());
        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
    }
}
void TransactionRepository::remove(int id){
    AbstractRepository::remove("id_transaction", "Transaction", id);
}
std::vector<Transaction> TransactionRepository::transactionView() const{
    std::vector<Transaction> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM transaction_view")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        try{
            Transaction t = Transaction(query.value(0).toInt(),
                                        query.value(1).toDate(),
                                        query.value(2).toDouble(),
                                        query.value(3).toInt(),
                                        query.value(4).toInt(),
                                        query.value(5).toString(),
                                        // skip the category and the icon
                                        query.value(8).toString());
            res.push_back(t);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}
std::vector<Transaction> TransactionRepository::getTransactionsForAccount(const int id_account) const{
    if(id_account <= 0){
        throw std::invalid_argument("The account's id is invalid!");
    }
    std::vector<Transaction> res;
    QSqlQuery query;
    query.prepare("SELECT * FROM \"Transaction\" WHERE id_account = :id OR \"id_accountTo\" = :id");
    query.bindValue(":id", id_account);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        try{
            Transaction t = Transaction(query.value(0).toInt(),
                                        query.value(1).toDate(),
                                        query.value(2).toDouble(),
                                        query.value(3).toInt(),
                                        query.value(4).toInt(),
                                        query.value(5).toString(),
                                        // skip the category and the icon
                                        query.value(8).toString());
            res.push_back(t);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}

void TransactionRepository::updateTransactionCategory(const int id, const QString& category, const QString& icon){
    if(id <= 0)
        throw std::invalid_argument("updateTransactionCategory: the id is invalid!");
    QSqlQuery query;
    query.prepare("UPDATE public.\"Transaction\" SET icon = :icon, category = :category WHERE id_transaction = :id");
    query.bindValue(":icon", icon);
    query.bindValue(":category", category);
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
}
std::pair<QString, QString> TransactionRepository::getTransactionCategoryIcon(const int id){
    if(id <= 0)
        throw std::invalid_argument("getTransactionCategoryIcon: the id is invalid!");
    QSqlQuery query;
    query.prepare("SELECT category, icon FROM public.\"Transaction\" WHERE id_transaction = :id");
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    if (query.next())
        return std::make_pair(query.value(0).toString(), query.value(1).toString());
    else
        return std::make_pair(QString(), QString());
}
std::vector<std::pair<QString, double>> TransactionRepository::getSpendingChartData() const{
    std::vector<std::pair<QString, double>> res;
    QSqlQuery query;
    if(!query.exec("SELECT category, SUM(amount) FROM \"Transaction\" GROUP BY category")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        const std::pair<QString, double> p = std::make_pair(query.value(0).toString(), query.value(1).toDouble());
        res.push_back(p);
    }
    return res;
}

std::vector<std::pair<QDate, double>> TransactionRepository::getMonthlySpendingHistory(const int accountId) const {
    std::vector<std::pair<QDate, double>> history;
    QSqlQuery query;
    // PostgreSQL Query:
    // 1. Filter by account
    // 2. Filter only "Money Out" (id_account)
    // 3. Group by Month (YYYY-MM)
    // 4. Order by Date ASC (Oldest -> Newest)
    query.prepare(R"(
        SELECT TO_CHAR(date_transaction, 'YYYY-MM'), SUM(amount)
        FROM public."Transaction"
        WHERE id_account = :id
        GROUP BY TO_CHAR(date_transaction, 'YYYY-MM')
        ORDER BY TO_CHAR(date_transaction, 'YYYY-MM') ASC
        LIMIT 12
    )");
    query.bindValue(":id", accountId);
    if(query.exec()) {
        while(query.next()) {
            const QDate date = QDate::fromString(query.value(0).toString(), "yyyy-MM");
            const std::pair<QDate, double> p = std::make_pair(date, query.value(1).toDouble());
            history.push_back(p);
        }
    }
    return history;
}
