#include "transactionrepository.h"
#include "entities/transaction.h"

TransactionRepository::TransactionRepository(){}

TransactionRepository::~TransactionRepository(){}

std::vector<std::shared_ptr<Entity>> TransactionRepository::getAll() const {
    std::vector<std::shared_ptr<Entity>> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM \"Transaction\"")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        int id = query.value(0).toInt();
        QDate date = query.value(1).toDate();
        double amount = query.value(2).toDouble();
        int id_account = query.value(3).toInt();
        int id_accountTo = query.value(4).toInt();
        try{
            auto transact = std::make_shared<Transaction>(id, date, amount, id_account, id_accountTo);
            res.push_back(transact);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}
void TransactionRepository::insert(std::shared_ptr<Entity> entity) {
    if(entity == nullptr){
        return;
    }
    if(Transaction* tran = dynamic_cast<Transaction*>(entity.get()); tran != nullptr){
        QSqlQuery query;
        query.prepare(QString("INSERT INTO public.\"Transaction\" ") +
                      "(date_transaction, amount, id_account, \"id_accountTo\")" +
                      "VALUES(:date_transaction, :amount, :id_account, :id_accountTo);");
        query.bindValue(":date_transaction", tran->getDate());
        query.bindValue(":amount", tran->getAmount());
        query.bindValue(":id_account", tran->getIdAccount());
        query.bindValue(":id_accountTo", tran->getIdAccountTo());
        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
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
                      "\"id_accountTo\" = :id_accountTo "
                      "WHERE id_transaction = :id"
                      );
        query.bindValue(":date_transaction", tran->getDate());
        query.bindValue(":amount", tran->getAmount());
        query.bindValue(":id_account", tran->getIdAccount());
        query.bindValue(":id_accountTo", tran->getIdAccountTo());
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
                                        query.value(4).toInt());
            res.push_back(t);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}
