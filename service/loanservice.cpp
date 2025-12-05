#include "loanservice.h"
#include "entities/loan.h"

LoanService::LoanService() {
    m_loan_repo = std::make_shared<LoanRepository>(LoanRepository());
}
void LoanService::getAll(QTextBrowser* browser, QTableWidget *table)const{
    std::vector<std::shared_ptr<Entity>> res = m_loan_repo->getAll();
    if(table != nullptr){
        table->setColumnCount(6);
        table->setHorizontalHeaderLabels({"id", "Account id", "Issue date", "Usage date", "Percent", "Amount"});
        table->setRowCount(res.size());
    }
    if(browser != nullptr)
        browser->append("id   Account id   Issue date   Usage date   Percent   Amount\n");
    for(size_t i = 0;i < res.size(); ++i){
        const auto ent = res[i];
        Loan* loan = dynamic_cast<Loan*>(ent.get());
        if(loan){
            qDebug() << *loan;
            if(browser != nullptr)
                browser << *loan;
            if(table != nullptr){
                table->setItem(i, 0, new QTableWidgetItem(QString::number(loan->getId())));
                table->item(i,0)->setFlags(table->item(i,0)->flags() & ~Qt::ItemIsEditable);
                table->setItem(i, 1, new QTableWidgetItem(QString::number(loan->getId_account())));
                table->setItem(i, 2, new QTableWidgetItem(loan->getIssue_date().toString()));
                table->setItem(i, 3, new QTableWidgetItem(loan->getUsage_date().toString()));
                table->setItem(i, 4, new QTableWidgetItem(QString::number(loan->getPercent())));
                table->setItem(i, 5, new QTableWidgetItem(QString::number(loan->getAmount())));
            }
        }
    }
}
 // file loanReport
void LoanService::putDataReportWithAllLoans(QtRPT* report) const{
    // devide the values by columns
    QVector<QString> loan_id, id_account, issue_date, percent, usage_date, amount;
    std::vector<std::shared_ptr<Entity>> allLoans = m_loan_repo->getAll();
    for(const auto& ent : allLoans){
        Loan *loan = dynamic_cast<Loan*>(ent.get());
        loan_id.push_back(QString::number(loan->getId()));
        id_account.push_back(QString::number(loan->getId_account()));
        issue_date.push_back(loan->getIssue_date().toString());
        percent.push_back(QString::number(loan->getPercent()));
        usage_date.push_back(loan->getUsage_date().toString());
        amount.push_back(QString::number(loan->getAmount()));
    }

    connect(report, &QtRPT::setValue, [=](const int recno, const QString paramname,
                                          QVariant& paramValue, const int reportpage){
        Q_UNUSED(reportpage);
        if(paramname == "id_loan"){
            paramValue = loan_id[recno];
        }
        if(paramname == "id_account"){
            paramValue = id_account[recno];
        }
        if(paramname == "issue_date"){
            paramValue = issue_date[recno];
        }
        if(paramname == "percent"){
            paramValue = percent[recno];
        }
        if(paramname == "usage_date"){
            paramValue = usage_date[recno];
        }
        if(paramname == "amount"){
            paramValue = amount[recno];
        }
    });
}
void LoanService::loanReportWithAllLoans(QMainWindow* window) const{
    size_t record_count = m_loan_repo->getAll().size();
    getReport(window, [&](QtRPT* report){ putDataReportWithAllLoans(report); }, record_count);
}

int LoanService::insertLoan(int id_account, QDate issue_date,
                QDate usage_date,double percent,
                double amount){
    std::shared_ptr<Loan> loan = std::make_shared<Loan>();
    bool isAccount = isPresent(id_account, m_account_repo.get());
    bool ok = true;
    try{
        loan->setAmount(amount);
        loan->setId_account(id_account);
        loan->setIssue_date(issue_date);
        loan->setUsage_date(usage_date);
        loan->setPercent(percent);
    }catch(const std::invalid_argument& e){
        ok = false;
        qDebug() << e.what();
    }
    if(ok && isAccount){
        m_loan_repo->insert(loan);
        return loan->getId();
    }
    else
        throw std::invalid_argument("The loan is invalid!");
}
void LoanService::updateLoan(int id, int id_account, QDate issue_date,
                QDate usage_date,double percent,
                double amount){
    if(!isPresent(id_account, m_account_repo.get())){
        throw std::invalid_argument("Update: There is no such account!");
    }
    // this may throw
    auto loan = std::make_shared<Loan>(id, id_account, issue_date, usage_date, percent, amount);
    m_loan_repo->update(loan);
}
void LoanService::deleteObj(const int id){
    deleteHelper(id, m_loan_repo.get());
}
void LoanService::getTotalEarnedMoney(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_loan_repo->getTotalEarnedMoney(getCurYear());
    browser->append("percent   amount   income\n");
    size_t size = res.size();
    for(size_t i = 0; i<size; ++i){
        const auto total = res[i];
        if(i == size-1){ // total sum
            browser->append("            " +
                            QString::number(total.income) + "\n");
            break;
        }
        browser->append(QString::number(total.percent) + "   " + QString::number(total.amount) + "   " +
                        QString::number(total.income) + "\n");
    }
}
void LoanService::getIndividualLoans(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_loan_repo->getIndividualLoans();
    browser->append("id_account   amount   currency   id_client   total_loan   percent_of_total\n");
    for(const auto& ent : res){
        browser->append(QString::number(ent.acc.getId()) + "   " +
                        QString::number(ent.acc.getAmount()) + "   " +
                        ent.acc.getCurrency() + "   " +
                        QString::number(ent.acc.getClientId()) + "   " +
                        QString::number(ent.totalLoan) + "   " +
                        QString::number(ent.percent_of_total) + "\n");
    }
}
void LoanService::getAverageLegalLoans(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_loan_repo->getAverageLegalLoans();
    browser->append("id_client   average loan\n");
    for(const auto& ent : res){
        browser->append(QString::number(ent.id_client) +
                        "   " + QString::number(ent.average) + "\n");
    }
}
void LoanService::getLoanClientView(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_loan_repo->getLoanClientView();
    browser->append("id_loan   legal_address   loan_amount\n");
    for(const auto& lc : res){
        browser->append(QString::number(lc.id_loan) + "   "
                        + lc.legal_address + "   "
                        + QString::number(lc.loan_amount) + "\n");
    }
}
void LoanService::getLoanView(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    std::vector<Loan> res = m_loan_repo->getLoanView();
    browser->append("id   Account id   Issue date   Usage date   Percent   Amount\n");
    for(const Loan& loan : res){
        browser << loan;
    }
}
