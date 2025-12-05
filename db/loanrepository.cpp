#include "loanrepository.h"
#include "entities/loan.h"

LoanRepository::LoanRepository(){}

LoanRepository::~LoanRepository(){}

std::vector<std::shared_ptr<Entity>> LoanRepository::getAll() const {
    std::vector<std::shared_ptr<Entity>> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM \"Loan\" ORDER BY id_loan")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        int id = query.value(0).toInt();
        int id_account = query.value(1).toInt();
        QDate issue_date = query.value(2).toDate();
        QDate usage_date = query.value(3).toDate();
        double percent = query.value(4).toDouble();
        double amount = query.value(5).toDouble();
        try{
            auto loan = std::make_shared<Loan>(id, id_account, issue_date, usage_date, percent, amount);
            res.push_back(loan);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}

std::shared_ptr<Entity> LoanRepository::getById(const int id) const{
    QSqlQuery query;
    query.prepare("SELECT * FROM \"Loan\" WHERE id_loan = :id");
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    if(query.next()){
        int id = query.value(0).toInt();
        int id_account = query.value(1).toInt();
        QDate issue_date = query.value(2).toDate();
        QDate usage_date = query.value(3).toDate();
        double percent = query.value(4).toDouble();
        double amount = query.value(5).toDouble();
        try{
            auto loan = std::make_shared<Loan>(id, id_account, issue_date, usage_date, percent, amount);
            return loan;
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return nullptr; // not found
}

void LoanRepository::insert(std::shared_ptr<Entity> entity) {
    if(entity == nullptr){
        return;
    }
    if(Loan* loan = dynamic_cast<Loan*>(entity.get()); loan != nullptr){
        QSqlQuery query;
        query.prepare(QString("INSERT INTO public.\"Loan\" ") +
                      "(id_account, issue_date, usage_date, percent, amount)" +
                      "VALUES(:id_account, :issue_date, :usage_date, :percent, :amount);");
        query.bindValue(":id_account", loan->getId_account());
        query.bindValue(":issue_date", loan->getIssue_date());
        query.bindValue(":usage_date", loan->getUsage_date());
        query.bindValue(":percent", loan->getPercent());
        query.bindValue(":amount", loan->getAmount());
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
void LoanRepository::update(std::shared_ptr<Entity> entity){
    if(entity == nullptr){
        return;
    }
    if(Loan* loan = dynamic_cast<Loan*>(entity.get()); loan != nullptr){
        QSqlQuery query;
        query.prepare("UPDATE public.\"Loan\" "
                      "SET id_account = :id_account, "
                      "issue_date = :issue_date, "
                      "usage_date = :usage_date, "
                      "percent = :percent, "
                      "amount = :amount "
                      "WHERE id_loan = :id"
                      );
        query.bindValue(":id_account", loan->getId_account());
        query.bindValue(":issue_date", loan->getIssue_date());
        query.bindValue(":usage_date", loan->getUsage_date());
        query.bindValue(":percent", loan->getPercent());
        query.bindValue(":amount", loan->getAmount());
        query.bindValue(":id", loan->getId());
        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
    }
}
void LoanRepository::remove(int id){
    AbstractRepository::remove("id_loan", "Loan", id);
}
/*
 prints the total amount received by the bank since the beginning of the current year
*/
std::vector<LoanRepository::totalMoney> LoanRepository::getTotalEarnedMoney(int year){
    std::vector<totalMoney> res;
    QSqlQuery query;
    query.prepare("SELECT * FROM EarnedMoney(:year)");
    query.bindValue(":year", year);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        totalMoney tm;
        tm.percent = query.value(0).toDouble();
        tm.amount = query.value(1).toDouble();
        tm.income = query.value(2).toDouble();
        res.push_back(tm);
    }
    return res;
}
/*
Gets data on loans issued to individuals. Displays the value for each
by the type of loans expressed as a percentage of the total amount of loans issued.*/
std::vector<LoanRepository::individualLoans> LoanRepository::getIndividualLoans() const{
    std::vector<individualLoans> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM IndividualLoans()")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        individualLoans ent;
        try{
            ent.acc.setId(query.value(0).toInt());
            ent.acc.setAmount(query.value(1).toDouble());
            ent.acc.setCurrency(query.value(2).toString());
            ent.acc.setClientId(query.value(3).toInt());
        }catch(const std::invalid_argument& e){
            qDebug() << "Failed to create an account: " << e.what();
            continue;
        }
        ent.totalLoan = query.value(4).toDouble();
        ent.percent_of_total = query.value(5).toDouble();
        res.push_back(ent);
    }
    return res;
}
/*
Gets the average amount of loans issued for each legal entity. Prints the data
in the descending order of values.
*/
std::vector<LoanRepository::legalLoans> LoanRepository::getAverageLegalLoans() const{
    std::vector<legalLoans> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM Legal_loans()")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        legalLoans ent;
        ent.id_client = query.value(0).toInt();
        ent.average = query.value(1).toDouble();
        res.push_back(ent);
    }
    return res;
}
std::vector<LoanRepository::loanClientView> LoanRepository::getLoanClientView() const{
    std::vector<loanClientView> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM loan_client_view")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        loanClientView lc;
        lc.id_loan = query.value(0).toInt();
        lc.legal_address = query.value(1).toString();
        lc.loan_amount = query.value(2).toDouble();
        res.push_back(lc);
    }
    return res;
}
std::vector<Loan> LoanRepository::getLoanView() const{
    std::vector<Loan> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM loan_view")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        try{
            Loan loan(query.value(0).toInt(), query.value(1).toInt(),
                      query.value(2).toDate(), query.value(3).toDate(),
                      query.value(4).toDouble(), query.value(5).toDouble());
            res.push_back(loan);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}
