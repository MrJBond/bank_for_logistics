#include "clientservice.h"

ClientService::ClientService() {
    m_client_repo = std::make_shared<ClientRepository>(ClientRepository());
    m_transaction_repo = std::make_shared<TransactionRepository>(TransactionRepository());
    m_loanRecommender = new LoanRecommender();
    m_chatBot = new ChatBot();
    m_faceIdService = new FaceIdService();
    connect(m_loanRecommender, &LoanRecommender::finalLoanAmountReady,
            this, &ClientService::handleFinalLoanAmount);

    connect(m_loanRecommender, &LoanRecommender::loanRejected,
            this, &ClientService::handleLoanRejection);

    connect(m_loanRecommender, &LoanRecommender::networkError,
            this, &ClientService::handleNetworkFailure);

    connect(m_chatBot, &ChatBot::chatReplyReady,
            this, &ClientService::handleChatReply);

    connect(m_chatBot, &ChatBot::networkError,
            this, &ClientService::handleNetworkFailure);

    connect(m_faceIdService, &FaceIdService::vectorCalculated,
            this, &ClientService::handleUserFaceVector);

    connect(m_faceIdService, &FaceIdService::networkError,
            this, &ClientService::handleNetworkFailure);

    connect(m_loanRecommender, &LoanRecommender::recommendationReady,
            this, &ClientService::handleRecommendation);
}
void ClientService::getAll(QTextBrowser* browser, QTableWidget *table)const{
    std::vector<std::shared_ptr<Entity>> res = m_client_repo->getAll();
    if(table != nullptr){
        table->setColumnCount(7);
        table->setHorizontalHeaderLabels({"id", "Name", "Address", "BossName", "BossPhone", "AccountantName", "AccountantPhone"});
        table->setRowCount(res.size());
    }
    if(browser != nullptr)
        browser->append("id   Name   Address   BossName   BossPhone   AccountantName   AccountantPhone\n");
    for(size_t i = 0;i < res.size(); ++i){
        const auto ent = res[i];
        Client* client = dynamic_cast<Client*>(ent.get());
        if(client){
            qDebug() << *client;
            if(browser != nullptr)
                browser << *client;
            if(table != nullptr){
                table->setItem(i, 0, new QTableWidgetItem(QString::number(client->getId())));
                table->item(i,0)->setFlags(table->item(i,0)->flags() & ~Qt::ItemIsEditable);
                table->setItem(i, 1, new QTableWidgetItem(QString(client->getName().c_str())));
                table->setItem(i, 2, new QTableWidgetItem(QString(client->getAddress().c_str())));
                table->setItem(i, 3, new QTableWidgetItem(QString(client->getBossName().c_str())));
                table->setItem(i, 4, new QTableWidgetItem(QString(client->getBossPhone().c_str())));
                table->setItem(i, 5, new QTableWidgetItem(QString(client->getAccountantName().c_str())));
                table->setItem(i, 6, new QTableWidgetItem(QString(client->getAccountantPhone().c_str())));
            }
        }
    }
}
void ClientService::getClientsWithTotalSum(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    auto res = m_client_repo->getClientsWithTotalSum();
    browser->append("id   Name   Total sum\n");
    for(const auto& ent : res){
        browser->append(QString::number(ent.id_client) + "   " +
                            ent.name + "   " + QString::number(ent.total_sum) + '\n');
    }
}
// file clientTransaction
void ClientService::putDataClientsWithTotalSumReport(QtRPT* report,
                                                     QVector<QString> id_client,
                                                     QVector<QString> name_client,
                                                     QVector<QString> total_sum) const{
    connect(report, &QtRPT::setValue, [=](const int recno, const QString paramname,
                                          QVariant& paramValue, const int reportpage){
        Q_UNUSED(reportpage);
        if(paramname == "id_client"){
            paramValue = id_client[recno];
        }
        if(paramname == "name_client"){
            paramValue = name_client[recno];
        }
        if(paramname == "total_sum"){
            paramValue = total_sum[recno];
        }
    });
}
void ClientService::getClientsWithTotalSumReport(QMainWindow* window) const{
    auto res = m_client_repo->getClientsWithTotalSum();
    QVector<QString> id_client, name_client, total_sum;
    for(const auto& ent : res){
        id_client.push_back(QString::number(ent.id_client));
        name_client.push_back(ent.name);
        total_sum.push_back(QString::number(ent.total_sum));
    }
    // get the number of rows
    size_t record_count = id_client.size();
    getReport(window, [&](QtRPT* report){ putDataClientsWithTotalSumReport(report,
                                                                      id_client, // pass from this function
                                                                      name_client,
                                                                      total_sum); }, record_count);
}


// for nested report
void ClientService::putDataSubReport(QtRPT* report, std::vector<Account> accs) const{
    std::vector<QString> id_account, amount, currency, id_client;
    for(const Account& acc : accs){
        id_account.push_back(QString::number(acc.getId()));
        amount.push_back(QString::number(acc.getAmount()));
        currency.push_back(acc.getCurrency());
        id_client.push_back(QString::number(acc.getClientId()));
    }
    connect(report, &QtRPT::setValue, [=](const int recno, const QString paramname,
                                          QVariant& paramValue, const int reportpage){
        Q_UNUSED(reportpage);
        if(paramname == "id_account"){
            paramValue = id_account[recno];
        }
        if(paramname == "amount"){
            paramValue = amount[recno];
        }
        if(paramname == "currency"){
            paramValue = currency[recno];
        }
        if(paramname == "id_client"){
            paramValue = id_client[recno];
        }
    });

}
// with nested report
void ClientService::putDataClientsAccountsReport(QtRPT* report, QMainWindow* window) {
    std::vector<std::shared_ptr<Entity>> res = m_client_repo->getAll();
    std::vector<QString> id_client, name_client, legal_address,
        boss_name, boss_phone, accountant_name, accountant_phone;

    std::vector<std::shared_ptr<QtRPT>> subReports;
    double totalAccountAmount = 0.;
    for(const auto& ent : res){
        Client* client = dynamic_cast<Client*>(ent.get());
        if(client){
            id_client.push_back(QString::number(client->getId()));
            name_client.push_back(client->getName().c_str());
            legal_address.push_back(client->getAddress().c_str());
            boss_name.push_back(client->getBossName().c_str());
            boss_phone.push_back(client->getBossPhone().c_str());
            accountant_name.push_back(client->getAccountantName().c_str());
            accountant_phone.push_back(client->getAccountantPhone().c_str());

            std::vector<Account> accs = m_client_repo->getAccountsForClient(client->getId());
            std::for_each(accs.begin(), accs.end(), [&](const Account& a){
                    totalAccountAmount += Entity::toDollar(a.getAmount(), a.getCurrency());
            });
            // put the accs into subreports (creating the reports)
            // build the subreport
            // set the name of the report
            bool ok = true;
            try{
                setFileName("nestedReport.xml");
            }
            catch(const std::runtime_error& e){
                ok = false;
                qDebug() << e.what();
            }
            if(ok){
                size_t record_count = accs.size();
                QtRPT* subreport = nullptr;
                if(!buildReport(window, subreport)){
                    return;
                }
                if(subreport){ // if we are here, it won't be nullptr anyway, so we can get rid of this if
                    connect(subreport, &QtRPT::setDSInfo, [=](DataSetInfo& dsinfo){
                        dsinfo.recordCount = record_count;
                    });
                    putDataSubReport(subreport, accs);
                    subReports.push_back(std::shared_ptr<QtRPT>(subreport));
                }
            }
        }

    }
    // reset the name of the report back to the main report's name
    setFileName("withNested.xml");

    connect(report, &QtRPT::setValue, [=](const int recno, const QString paramname,
                                          QVariant& paramValue, const int reportpage){
        Q_UNUSED(reportpage);
        if(paramname == "id_client"){
            paramValue = id_client[recno];
        }
        if(paramname == "name_client"){
            paramValue = name_client[recno];
        }
        if(paramname == "legal_address"){
            paramValue = legal_address[recno];
        }
        if(paramname == "boss_name"){
            paramValue = boss_name[recno];
        }
        if(paramname == "boss_phone"){
            paramValue = boss_phone[recno];
        }
        if(paramname == "accountant_name"){
            paramValue = accountant_name[recno];
        }
        if(paramname == "accountant_phone"){
            paramValue = accountant_phone[recno];
        }
        if(paramname == "total_clients"){
            paramValue = id_client.size();
        }
        if(paramname == "total_amount"){
            paramValue = totalAccountAmount;
        }
    });
    // subreport
    connect(report, &QtRPT::setValueImage, [=](const int recno, const QString paramname,
                                               QImage& paramValue, const int reportpage){
        Q_UNUSED(reportpage);
        if(paramname == "subreport"){
            // convert subReport to pdf and print
            QPrinter printer(QPrinter::HighResolution);
            printer.setOutputFormat(QPrinter::PdfFormat);
            // Save to a temporary file
            printer.setOutputFileName(QString("subReport") + QString::number(recno) + ".pdf");

            subReports[recno]->printPDF(printer.outputFileName(), false); // false - not to open
            QString pdfFilePath = "./" + QString("subReport") + QString::number(recno) + ".pdf";

            QPdfDocument pdfDoc;
            if (pdfDoc.load(pdfFilePath) != QPdfDocument::Error::None) {
                qWarning() << "Failed to load PDF file:" << pdfFilePath;
                return;
            }
            // convert to image
            // add all pages of the subreport to the image
            int totalPages = pdfDoc.pageCount();
            int dpi = 500; // dot per inch
            QSize combinedSize(0, 0);
            for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
                QSizeF pageSize = pdfDoc.pagePointSize(pageIndex);
                QSize imageSize = pageSize.toSize() * dpi / 72;
                combinedSize.setHeight(combinedSize.height() + imageSize.height());
                combinedSize.setWidth(qMax(combinedSize.width(), imageSize.width()));
            }
            QImage pdfPageImage(combinedSize, QImage::Format_ARGB32);
            QPainter painter(&pdfPageImage);
            int yOffset = 0;

            for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
                QSizeF pageSize = pdfDoc.pagePointSize(pageIndex);
                QSize imageSize = pageSize.toSize() * dpi / 72;
                QImage pageImage = pdfDoc.render(pageIndex, imageSize);

                painter.drawImage(QPoint(0, yOffset), pageImage);
                yOffset += imageSize.height();
            }
            painter.end();

            if (pdfPageImage.isNull()) {
                qWarning() << "Failed to render PDF page.";
                return;
            } // img of the subreport
           paramValue = pdfPageImage;
        }

    });
}
void ClientService::getClientsAccountsReport(QMainWindow* window) {
    size_t record_count = m_client_repo->getAll().size();
    getReport(window, [&](QtRPT* report){putDataClientsAccountsReport(report, window);}, record_count);
}

int ClientService::insertClient(QString name, QString address, QString bossName,
                  QString bossPhone, QString accountantName,
                                 QString accountantPhone){
    if(name == "")
        throw std::invalid_argument("The client's name must be non-empty and unique!");
    if(name == "" && address == "" && bossName == ""
        && bossPhone == "" && accountantName == "" && accountantPhone == ""){
        throw std::invalid_argument("The client is invalid!");
    }
    std::shared_ptr<Client> client = std::make_shared<Client>();
    client->setName(name.toStdString());
    client->setAddress(address.toStdString());
    client->setBossName(bossName.toStdString());
    client->setBossPhone(bossPhone.toStdString());
    client->setAccountantName(accountantName.toStdString());
    client->setAccountantPhone(accountantPhone.toStdString());
    m_client_repo->insert(client);
    return client->getId();
}
void ClientService::updateClient(int id, QString name, QString address, QString bossName,
                  QString bossPhone, QString accountantName,
                  QString accountantPhone){
    if(name == "")
        throw std::invalid_argument("The client's name must be non-empty and unique!");
    if(name == "" && address == "" && bossName == ""
        && bossPhone == "" && accountantName == "" && accountantPhone == ""){
        throw std::invalid_argument("Update: The client is invalid!");
    }
    // this may throw
    auto client = std::make_shared<Client>(id, address.toStdString(),
                                           name.toStdString(),
                                           bossName.toStdString(),
                                           bossPhone.toStdString(),
                                           accountantName.toStdString(),
                                           accountantPhone.toStdString());
    m_client_repo->update(client);
}
void ClientService::deleteObj(const int id){
    deleteHelper(id, m_client_repo.get());
}
void ClientService::getClientsLegalIndividual(QTextBrowser* browser) const{
    const auto res = m_client_repo->getClientsLegalOrIndividual();
    browser->append("id   legal_address   name   type\n");
    for(const auto& c : res){
        browser->append(QString::number(c.id_client) + "   "
                        + c.legal_address + "   "
                        + c.name + "   " + c.type + '\n');
    }
}
void ClientService::getLegalClientsWithTotalSum(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_client_repo->getLegalClientsWithTotalSum(getCurYear());
    browser->append("id   sum\n");
    for(const auto& c : res){
        browser->append(QString::number(c.id_client) + "   " +
                        QString::number(c.total_sum) + '\n');
    }
}
void ClientService::getDirectorView(QTextBrowser* browser) const{
    if(browser == nullptr){
        return;
    }
    const auto res = m_client_repo->getDirectorView();
    browser->append("id_client   id_account   name_client   amount   currency\n");
    for(const auto& ent : res){
        browser->append(QString::number(ent.id_client) + "   "
                        + QString::number(ent.id_ccount) + "   "
                        + ent.name + "   "
                        + QString::number(ent.amount) + "   "
                        + ent.currency + "\n");
    }
}
bool ClientService::isClientPresent(const int id) const{
    return isPresent(id, m_client_repo.get()); // throws
}
std::vector<Account> ClientService::getAccountsForClient(const int id_client) const{
    return m_client_repo->getAccountsForClient(id_client); // thows
}
/****************************************************
     *                      AI Lab2
****************************************************/
void ClientService::recommendLoanAmount(const int id) const{
    m_loanRecommender->recommendLoanAmount(id, m_client_repo.get());
}
std::vector<Transaction> ClientService::listTransactions(const int id_client) const{
    const std::vector<Account> accs = m_client_repo->getAccountsForClient(id_client); // might throw
    std::vector<Transaction> res;
    for(const Account& a : accs){
        std::vector<Transaction> tran = m_transaction_repo->getTransactionsForAccount(a.getId());
        res.insert(std::end(res), std::begin(tran), std::end(tran));
    }
    // remove duplicates if any
    std::sort(res.begin(), res.end());
    res.erase(std::unique(res.begin(), res.end()), res.end());
    return res;
}
void ClientService::handleFinalLoanAmount(double amount)
{
    // Bot reply
    const QString reply = QString("Based on your history, I can recommend up to $%1.").arg(amount);
    // The result has arrived!
    qDebug() << "ClientService: Suggested Loan: " << amount;
    emit finalLoanAmount(amount);
    emit chatReplyString(reply);
}

void ClientService::handleLoanRejection()
{
    // The rejection message has arrived!
    qDebug() << "ClientService: Loan Application Rejected.";
    // Bot reply
    const QString reply = "I've analyzed your profile. Unfortunately, based on your current financial stability and debt, I cannot recommend a loan at this time.";
    emit chatReplyString(reply);
    emit loanRejection();
}

void ClientService::handleNetworkFailure(const QString& errorString)
{
    // A network error occurred
    qDebug() << "ClientService: Network Error:" << errorString;
    emit networkFailure(errorString);
}

void ClientService::sendToPythonBot(const QString& userText){
    m_chatBot->requestChat(userText);
}

// Bot's Entry Point
void ClientService::handleUserMessage(const QString& msg) {
    // CASE A: Survey is running -> Intercept the message!
    if (m_isSurveyActive) {
        // Process the answer locally
        processSurveyAnswer(msg);
        return; // STOP HERE. Do not send to Python.
    }
    // CASE B: Normal Chat -> Send to Python
    sendToPythonBot(msg);
}

void ClientService::handleChatReply(const QString& intent, const QString& reply){
    qDebug() << "ClientService: " << intent << " " << reply;
    emit chatReplyString(reply);
    try{
        if(intent == "check_balance"){
            const std::vector<Account> balance = m_client_repo->getAccountsForClient(m_session->getUserId());
            if (balance.empty()) {
                emit chatReplyString("It looks like you don't have any active accounts with us yet.");
            } else {
                QString msg = "Here is the breakdown of your accounts:\n";
                for (const auto& acc : balance) {
                    msg += QString("- %1 Account: %2\n")
                               .arg(acc.getCurrency())
                               .arg(acc.getAmount(), 0, 'f', 2);
                }
                emit chatReplyString(msg); // Bot speaks the list
                emit balanceCheckResult(balance); // App shows the detailed table

                double total = 0;
                for(auto& acc : balance)
                    total += Entity::toDollar(acc.getAmount(), acc.getCurrency());
                // Smart Follow-up
                if (total > 50000)
                    emit chatReplyString("💡 You have a significant balance! You should consider opening a high-interest savings account.");
                else if (total < 100)
                    emit chatReplyString("⚠️ Your balance is running low. Would you like to check your loan eligibility? Just ask 'Can I get a loan?'");
            }
        }else if(intent == "request_loan_recommendation"){
            recommendLoanAmount(m_session->getUserId()); // 21 using a random id to test
        }
        else if(intent == "list_transactions"){
            const std::vector<Transaction> tran = listTransactions(m_session->getUserId());
            if (tran.empty()) {
                emit chatReplyString("I looked through your records, but I couldn't find any recent transactions.");
            } else {
                // --- "Talk" Logic ---
                double totalAmount = 0.0;
                for (const auto& t : tran) {
                    totalAmount += t.getAmount();
                }
                QString summary = QString("I found %1 recent transactions. The total volume is $%2.")
                                      .arg(tran.size())
                                      .arg(totalAmount, 0, 'f', 2);

                emit chatReplyString(summary); // Bot speaks the summary
                emit transactionListResult(tran); // App shows the detailed table
            }
        }
        else if (intent == "assess_risk") {
            startRiskSurvey();
        }
        else if(intent == "greeting"){
            // don't have to do anything here...
        }
        else if (intent == "gratitude") {
            // don't have to do anything here...
        }
        else if(intent == "unknown"){
            // don't have to do anything here...
        }else{}

    }catch(const std::runtime_error& e){
        qDebug() << "ChatBot ERROR: " << e.what();
        emit networkFailure(e.what());
    }catch(const std::invalid_argument& e){
        qDebug() << "ChatBot ERROR: " << e.what();
        emit networkFailure(e.what());
    }
}
void ClientService::handleRecommendation(double score, double averageMonthlyIncome){
    QString reply;
    if(score < 20){ /* Rejection is handled in a different function */}
    else if (score < 50) {
        reply = QString("You are eligible for a small loan.");
    } else {
        reply = QString("Great news! You have a strong financial profile.");
    }
    emit chatReplyString(reply);
    setupHealthScoreChart(score);
}

// --- SURVEY [Bot's asses_risk feature]
// 1. Starting the Survey
void ClientService::startRiskSurvey() {
    m_isSurveyActive = true;
    m_currentQuestionIndex = 0;
    m_riskScore = 0;
    // Ask the first question
    emit chatReplyString("Let's figure out your risk profile. " + m_surveyQuestions[0]);
}
// 2. Processing Answers
void ClientService::processSurveyAnswer(const QString& answer) {
    QString cleanAnswer = answer.toLower().trimmed();

    // --- Simple Logic to Calculate Score ---
    auto isAffirmative = [](const QString& text) -> bool{
        QString t = text.toLower().trimmed();
        return (t == "yes" || t == "y" || t == "yeah" || t == "sure" || t == "yup");
    };
    auto isNegative = [](const QString& text) -> bool{
        QString t = text.toLower().trimmed();
        return (t == "no" || t == "n" || t == "nope" || t == "nah");
    };
    // If it's NOT yes AND it's NOT no, it's garbage input
    if (!isAffirmative(answer) && !isNegative(answer)) {
        emit chatReplyString("I didn't quite get that. Please answer with 'Yes' or 'No'.");
        // We do NOT increment the question index
        // The user effectively stays on the current question
        return;
    }
    // Q1: Emergency Fund? (Yes = Good/Safe)
    if (m_currentQuestionIndex == 0) {
        if (isAffirmative(cleanAnswer)) m_riskScore += 10;
    }
    // Q2: Panic Sell? (Yes = Risk Averse/Safe)
    else if (m_currentQuestionIndex == 1) {
        if (isAffirmative(cleanAnswer)) m_riskScore += 10;
    }
    // Q3: Retiring soon? (Yes = Should be Safe)
    else if (m_currentQuestionIndex == 2) {
        if (isAffirmative(cleanAnswer)) m_riskScore += 10;
    }

    // --- Move to Next Question ---
    m_currentQuestionIndex++;

    if (m_currentQuestionIndex < m_surveyQuestions.size()) {
        // Ask the next question
        emit chatReplyString(m_surveyQuestions[m_currentQuestionIndex]);
    } else {
        // No more questions
        finishSurvey();
    }
}

// 3. Finishing Up
void ClientService::finishSurvey() {
    m_isSurveyActive = false; // Turn off interception

    QString resultMsg;
    if (m_riskScore >= 20) {
        resultMsg = "Analysis complete. You are a **Conservative Investor**. You prioritize safety over high returns. I recommend our Fixed Deposit accounts.";
    } else if (m_riskScore >= 10) {
        resultMsg = "Analysis complete. You are a **Balanced Investor**. You can handle some market fluctuation. I recommend our Index Funds.";
    } else {
        resultMsg = "Analysis complete. You are an **Aggressive Investor**. You are looking for growth. Have you checked our Stock Trading platform?";
    }

    emit chatReplyString(resultMsg);
}

ClientService::MonthlyData ClientService::getMonthlyIncomeExpenses(const int clientId, const QDate& startDate) const {
    const std::vector<Transaction> trans = listTransactions(clientId);  // throws runtime_error | invalid_argument
    const std::vector<Account> accs = m_client_repo->getAccountsForClient(clientId); // throws runtime_error | invalid_argument
    MonthlyData totalResult;
    for(const Account& a : accs){
        const int myAccountId = a.getId();
        MonthlyData accResult; // for account
        for (const Transaction& t : trans) {
            if (t.getDate() < startDate)
                continue; // Skip old transactions

            const QString monthKey = t.getDate().toString("yyyy-MM");
            const double amount = t.getAmount();

            // Ensure the month entry exists (initializes to {0.0, 0.0})
            if (!accResult.contains(monthKey))
                accResult[monthKey] = std::make_pair(0.0, 0.0);
            if(!totalResult.contains(monthKey))
                totalResult[monthKey] = std::make_pair(0.0, 0.0);

            // isAccountMine throws runtime_error | invalid_argument
            if (t.getIdAccountTo() == myAccountId
                && !m_client_repo->isAccountMine(clientId, t.getIdAccount())) { // prevent my_Acc to another_my_Acc
                // Money coming TO me -> INCOME (First element of pair)
                accResult[monthKey].first += amount;
                qDebug() << "Account: " << myAccountId << " : Income += " << amount;
            }
            else if (t.getIdAccount() == myAccountId
                       && !m_client_repo->isAccountMine(clientId, t.getIdAccountTo())) { // prevent my_Acc to another_my_Acc
                // Money coming FROM me -> EXPENSE (Second element of pair)
                accResult[monthKey].second += amount;
                qDebug() << "Account: " << myAccountId << " : Expense += " << amount;
            }
        }
        // sum all accounts, taking the currency into consideration
        const QList keys = accResult.keys();
        for(const QString& month : keys){
            totalResult[month].first += Entity::toDollar(accResult[month].first, a.getCurrency());
            totalResult[month].second += Entity::toDollar(accResult[month].second, a.getCurrency());
        }
    }
    return totalResult;
}
/********************************************************
                    CHARTS
 *********************************************************/
void ClientService::incomeExpensesChart(const int w, const int h) const{
    const int id = m_session->getUserId();
    if(id <= 0){
        const QString message = "User's id is invalid! id = " + QString::number(id);
        throw std::runtime_error(message.toStdString());
    }
    const QDate twelveMonthsAgo = QDate::currentDate().addYears(-1);
    const MonthlyData data = getMonthlyIncomeExpenses(id, twelveMonthsAgo); // throws runtime_error | invalid_argument
    qDebug() << data;

    // build chart
    QBarSet *barSet = new QBarSet("Net Balance");
    QStringList categories;
    for (auto it = data.begin(); it != data.end(); ++it) {
        const QString monthKey = it.key();
        // --- Combine Income and Expense into one number ---
        const double income = it.value().first;
        const double expense = it.value().second;
        const double net = income - expense;
        *barSet << net;
        QDate d = QDate::fromString(monthKey, "yyyy-MM");
        categories << d.toString("MMM");
    }
    auto toolTipText = [&](const int index) -> QString {
        // Retrieve the value of the bar using the index
        double amount = barSet->at(index);
        // Format the text
        QString toolTipText = QString("net: %1 ").arg(amount, 0, 'f', 2);
        return toolTipText;
    };
    QChartView* barChart = createBarChart(barSet, "Net Savings (Income - Expenses)", categories, toolTipText);
    createChartBox(barChart, w, h);
}
QLineSeries* ClientService::fetchBalanceHistory(const int id_client, QStringList& categories) const{
    const QDate twelveMonthsAgo = QDate::currentDate().addYears(-1);
    const MonthlyData data = getMonthlyIncomeExpenses(id_client, twelveMonthsAgo); // throws runtime_error | invalid_argument
    const double currentFinalBalance = m_client_repo->getTotalCurrentBalance(id_client); // throws runtime_error | invalid_argument
    // Calculate the "Starting Balance" (Balance 12 months ago)
    double totalNetChangeOverPeriod = 0.0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        const double income = it.value().first;
        const double expense = it.value().second;
        totalNetChangeOverPeriod += (income - expense);
    }
    // If I have 1000 now, and I saved 200 over the year, I started with 800.
    double runningBalance = currentFinalBalance - totalNetChangeOverPeriod;
    // Create the Series
    QLineSeries *series = new QLineSeries();
    series->setName("Balance History");

    int xIndex = 0; // We use 0, 1, 2... for the X-axis

    // Add the starting point (before the first month's data applies)
     series->append(xIndex++, runningBalance);
    // Iterate Forward to generate points
    // Since 'data' is a QMap, this loop iterates from Oldest Date -> Newest Date
    for (auto it = data.begin(); it != data.end(); ++it) {
        const double income = it.value().first;
        const double expense = it.value().second;
        const double netChange = income - expense;
        // Apply the change for this month
        runningBalance += netChange;
        // Add the point to the graph
        // X = Month Index, Y = Balance
        series->append(xIndex, runningBalance);
        xIndex++;
        categories << QDate::fromString(it.key(), "yyyy-MM").toString("MMM");
    }
    // Verify: runningBalance should now equal currentFinalBalance (roughly)
    qDebug() << runningBalance << " " << currentFinalBalance;
    return series;
}
void ClientService::balanceHistoryChart(const int w, const int h) const{
    /*
     * Shows the trend of wealth. "Am I getting richer or poorer over time?"
     */
    const int id = m_session->getUserId();
    if(id <= 0){
        const QString message = "User's id is invalid! id = " + QString::number(id);
        throw std::runtime_error(message.toStdString());
    }
    QStringList categories;
    QLineSeries* series = fetchBalanceHistory(id, categories); // throws runtime_error | invalid_argument
    // build chart
    QChart *chart = new QChart();
    QChartView* chartView = setChartAndAxisProperties(chart, series, "Balance History (Last 12 Months)", categories);
    createChartBox(chartView, w, h);
}
void ClientService::setupHealthScoreChart(const double score) const{
    // 1. Create the Series
    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.65); // Makes it a Donut (0.0 = Pie, 1.0 = Empty ring)

    // 2. Prepare the Data Slices
    // Slice 1: The actual score
    QPieSlice *sliceScore = series->append("Score", score);

    // Slice 2: The "empty" part (100 - score)
    QPieSlice *sliceRemainder = series->append("Remainder", 100.0 - score);

    // 3. Style the "Score" Slice
    sliceScore->setLabelVisible(false); // We will put a custom label in the center instead
    sliceScore->setBorderColor(Qt::transparent); // Remove ugly border lines

    // Dynamic Color Logic based on the score
    if (score >= 80) {
        sliceScore->setColor(QColor("#2ecc71")); // Green (Excellent)
    } else if (score >= 50) {
        sliceScore->setColor(QColor("#f1c40f")); // Yellow (Fair)
    } else {
        sliceScore->setColor(QColor("#e74c3c")); // Red (Poor)
    }

    // 4. Style the "Remainder" Slice
    sliceRemainder->setColor(QColor("#ecf0f1")); // Light Gray
    sliceRemainder->setBorderColor(Qt::transparent);
    sliceRemainder->setLabelVisible(false);

    // 5. Create and Configure the Chart
    QChart *chart = new QChart();
    QStringList categories;
    QChartView* chartView = setChartAndAxisProperties(chart, series, "Score chart", categories);

    // ---------------------------------------------------------
    //  Add the Number in the Center
    // ---------------------------------------------------------
    // QCharts doesn't have a native "center label", so we add a QLabel
    // on top of the view. We need to position it manually.

   // Check if we already created a label previously (to avoid duplicates)
    QLabel *scoreLabel = chartView->findChild<QLabel*>("centerLabel");
    if (!scoreLabel) {
        scoreLabel = new QLabel(chartView);
        scoreLabel->setObjectName("centerLabel");
        scoreLabel->setAlignment(Qt::AlignCenter);

        // Make it look nice
        QFont font;
        font.setPixelSize(30);
        font.setBold(true);
        scoreLabel->setFont(font);
    }

    // Set text
    scoreLabel->setText(QString::number(score, 'f', 0) + "%");

    // Center the label (This needs to happen when the chart resizes,
    // but for now we set it initially).
    scoreLabel->resize(100, 50);
    scoreLabel->move((chartView->width() - scoreLabel->width()) / 2,
                     (chartView->height() - scoreLabel->height()) / 2);
    scoreLabel->show();
    createChartBox(chartView, 700, 500);
}
/************************************************************
                FACE LOG IN
 *************************************************************/
// Helper: Euclidean Distance
double calculateDistance(const std::vector<double>& v1, const std::vector<double>& v2) {
    if (v1.size() != v2.size()) return 1000.0; // Mismatch error

    double sum = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        double diff = v1[i] - v2[i];
        sum += (diff * diff);
    }
    return std::sqrt(sum);
}
bool ClientService::verifyFaceLogin(const QString& currentFaceVectorJson) const {
    // 1. Parse the Current User's Vector (from the login camera scan)
    QJsonDocument doc = QJsonDocument::fromJson(currentFaceVectorJson.toUtf8());
    QJsonArray arr = doc.array();
    std::vector<double> currentVector;
    for(auto val : arr) currentVector.push_back(val.toDouble());

    // 2. Fetch ALL users' face vectors from DB
    std::vector<std::pair<int, QString>> faces;
    try {
        faces = m_client_repo->getFaces();
    } catch(const std::runtime_error& e) {
        qDebug() << e.what();
        return false;
    }

    // 3. Compare with every user in the database
    double bestDistance = 1000.0;
    int bestMatchId = -1;

    for(const auto& f : faces) {
        const int id = f.first;
        const QString dbJson = f.second;

        // Parse DB vector
        QJsonDocument dbDoc = QJsonDocument::fromJson(dbJson.toUtf8());
        QJsonArray dbArr = dbDoc.array();
        std::vector<double> dbVector;
        for(auto val : dbArr) dbVector.push_back(val.toDouble());

        // 4. Calculate Distance
        double dist = calculateDistance(currentVector, dbVector);

        // Log it for debugging (CRITICAL to see what values we are getting)
        qDebug() << "Comparing against User ID:" << id << " Distance:" << dist;

        // Facenet L2 Threshold is usually around 10.0
        // We track the BEST match, not just the first one under threshold
        if (dist < 10.0 && dist < bestDistance) {
            bestDistance = dist;
            bestMatchId = id;
        }
    }

    // 5. Final Decision
    if (bestMatchId != -1) {
        qDebug() << "Face Match Found! User ID:" << bestMatchId << " Best Distance:" << bestDistance;

        std::shared_ptr<Client> client = nullptr;
        try {
            auto c = m_client_repo->getById(bestMatchId);
            c ? client = std::dynamic_pointer_cast<Client>(c) : client = nullptr;
        } catch(const std::runtime_error& e) {
            qDebug() << e.what();
            return false;
        }

        if(!client) return false;

        // LOGIN SUCCESSFUL
        m_session->createSession(bestMatchId, QString(client->getName().c_str()));
        return true;
    }

    qDebug() << "Login Failed: No matching face found.";
    return false;
}
void ClientService::requestFaceVector(const QString& base64Image){
    m_faceIdService->requestFaceVector(base64Image);
}
void ClientService::handleUserFaceVector(const QString& vectorJson){
    /*
     *  SIGN UP
     */
    if(m_session->isLoggedIn()){
        // the session has already been created in MainWindow::attemptSignupBankUser
        try{
            m_client_repo->saveUserFace(m_session->getUserId(), vectorJson);
        }catch(const std::runtime_error& e){
            qDebug() << e.what();
        }
    }else{
    /*
     *  LOG IN
     *  the session hasn't been created yet
     */
        if(!verifyFaceLogin(vectorJson)){
            qDebug() << "The face hasn't been verified!";
        }else{
            qDebug() << "ClientService: logged in successfully!";
            emit faceLoginSuccessful();
        }
    }
}
