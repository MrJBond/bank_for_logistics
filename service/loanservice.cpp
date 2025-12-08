#include "loanservice.h"
#include "entities/loan.h"

LoanService::LoanService() {
    m_loan_repo = std::make_shared<LoanRepository>(LoanRepository());
    m_client_repo = std::make_shared<ClientRepository>(ClientRepository());
    m_loanRecommender = std::make_unique<LoanRecommender>();
    connect(m_loanRecommender.get(), &LoanRecommender::recommendationReady,
            this, &LoanService::handleRecommendation);
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
/*********************************************
            Take a loan
 *********************************************/

// Helper to calculate rate based on the Fuzzy Logic Score
double LoanService::calculateInterestRate(const double aiScore) const{
    // Base rate 5%. Every point below 100 adds 0.15% interest.
    // Max rate (at score 0) would be 20%.
    const double rate = 5.0 + ((100.0 - aiScore) * 0.15);
    return rate;
}
void LoanService::takeLoan(const int accountId, const double amount, const int durationMonths, const double aiScore) {
    QSqlDatabase *db = DbConnector::getInstance()->getDb();
    // 1. Start Transaction
    // Taking a loan involves TWO steps: creating the loan record AND adding money to the account.
    if (!db->transaction()) {
        throw std::runtime_error("Failed to start database transaction.");
    }
    try {
        // 2. Logic Calculations
        const QDate issueDate = QDate::currentDate();
        const QDate usageDate = issueDate.addMonths(durationMonths); // Calculates the end date
        const double interestRate = calculateInterestRate(aiScore);
        // 3. Insert Loan Record
        const int loan = insertLoan(accountId, issueDate, usageDate, interestRate, amount);
        // 4. Update Account Balance (Client receives the money)
        auto acc = std::dynamic_pointer_cast<Account>(m_account_repo->getById(accountId));
        const double currentBalance = acc->getAmount();
        const double newBalance = currentBalance + amount;
        acc->setAmount(newBalance);
        m_account_repo->update(acc);
        // 5. Commit Transaction
        if (!db->commit()) {
            throw std::runtime_error("Failed to commit transaction.");
        }
        qDebug() << "Loan approved! Amount:" << amount << " Rate:" << interestRate << "%";

    } catch (const std::exception& e) {
        db->rollback();
        throw; // Re-throw so the UI can show an error message
    }
}
void LoanService::requestLoan(const int accountId, const double amount, const int durationMonths){
    const int id = m_session->getUserId();
    if(id <= 0){
        const QString message = "User's id is invalid! id = " + QString::number(id);
        throw std::runtime_error(message.toStdString());
    }
    if(accountId <= 0){
        throw std::runtime_error("The account is invalid!");
    }
    if(!isPresent(accountId, m_account_repo.get())){
        throw std::runtime_error("There is no such account!");
    }
    if(!ClientRepository::isAccountMine(id, accountId)){
        throw std::runtime_error("The account is not yours!");
    }
    if(amount <= 0){
        throw std::runtime_error("The amount must be positive!");
    }
    if(durationMonths <= 0){
        throw std::runtime_error("The duration must be positive!");
    }

    // 1. SAVE THE CONTEXT
    // We store the parameters here so we can access them later
    // when the AI replies.
    m_pendingRequest.accountId = accountId;
    m_pendingRequest.amount = amount;
    m_pendingRequest.durationMonths = durationMonths;
    m_pendingRequest.isValid = true;
    qDebug() << "Loan Request Queued. Asking AI for approval...";
    m_loanRecommender->recommendLoanAmount(id, m_client_repo.get());
}
void LoanService::handleRecommendation(double score, double averageMonthlyIncome){
    // 1. Check if we actually have a pending request
    if (!m_pendingRequest.isValid) {
        qWarning() << "Received recommendation but no loan request was pending.";
        return;
    }
    qDebug() << "AI Response Received. Score:" << score;

    // 2. Apply Business Logic based on the AI result
    // Calculate the maximum allowed loan based on the bank's rule (e.g. 3x income * score%)
    double maxAllowedLoan = (averageMonthlyIncome * 3.0) * (score / 100.0);

    // 3. Decision Time
    if (score < 20.0) {
        // AI Rejected (Score too low)
        QString msg = "Loan rejected based on risk profile (Score: " + QString::number(score) + ")";
        emit loanResult(false, msg);
    }
    else if (m_pendingRequest.amount > maxAllowedLoan) {
        // AI Approved, but the user asked for too much money
        QString msg = "Loan amount too high. Based on your score, we can only offer up to " + QString::number(maxAllowedLoan);
        emit loanResult(false, msg);
    }
    else {
        // 4. SUCCESS - Execute the stored request
        try {
            takeLoan(m_pendingRequest.accountId,
                     m_pendingRequest.amount,
                     m_pendingRequest.durationMonths,
                     score); // Pass the score for interest rate calc

            emit loanResult(true, "Loan successfully approved and deposited!");

        } catch (const std::exception& e) {
            emit loanResult(false, "System Error: " + QString(e.what()));
        }
    }
    // 5. Cleanup
    m_pendingRequest.isValid = false; // Reset the state
}
