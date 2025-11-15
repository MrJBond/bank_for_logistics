#include "clientservice.h"

ClientService::ClientService() {
    m_client_repo = std::make_shared<ClientRepository>(ClientRepository());
    m_loanRecommender = new LoanRecommender();
    m_chatBot = new ChatBot();
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
}
void ClientService::getAllClients(QTextBrowser* browser, QTableWidget *table)const{
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
                if(a.getCurrency() == QString(EURO)){
                    totalAccountAmount += a.getAmount()*1.06;
                }
                else if(a.getCurrency() == QString(POUND)){
                    totalAccountAmount += a.getAmount()*1.27;
                }
                else if(a.getCurrency() == QString(HRYVNA)){
                    totalAccountAmount += a.getAmount()*0.024;
                }
                else{
                    totalAccountAmount += a.getAmount();
                }
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
void ClientService::deleteClient(int id){
    bool isPresentC = isPresent<Client>(id, [&](){return m_client_repo->getAll();});
    if(!isPresentC){
        throw std::invalid_argument("Delete: There is no such client!");
    }
    m_client_repo->remove(id);
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
bool ClientService::isClientPresent(const int id) {
    return isPresent<Client>(id, [&](){return m_client_repo->getAll();});
}
/****************************************************
     *                      AI Lab2
****************************************************/
void ClientService::recommendLoanAmount(const int id) const{
    double avg_income = 0., stability_metric = 0., existing_debt = 0.;
    try{
        avg_income = m_client_repo->averageMonthlyIncome(id);
        stability_metric = m_client_repo->incomeVolatility(id);
        existing_debt = m_client_repo->existingDebtLoad(id);
    }catch(const std::runtime_error& e){
        qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        qDebug() << e.what();
    }
    qDebug() << "avg_income: " << avg_income;
    qDebug() << "stability_metric: " << stability_metric;
    qDebug() << "existing_debt: " << existing_debt;
    m_loanRecommender->requestLoanRecommendation(avg_income, stability_metric, existing_debt);
}
void ClientService::handleFinalLoanAmount(double amount)
{
    // The result has arrived!
    qDebug() << "ClientService: Suggested Loan: " << amount;
    emit finalLoanAmount(amount);
}

void ClientService::handleLoanRejection()
{
    // The rejection message has arrived!
    qDebug() << "ClientService: Loan Application Rejected.";
    emit loanRejection();
}

void ClientService::handleNetworkFailure(const QString& errorString)
{
    // A network error occurred
    qDebug() << "ClientService: Network Error:" << errorString;
    emit networkFailure(errorString);
}

void ClientService::getBotResponse(const QString& userText){
    m_chatBot->requestChat(userText);
}
void ClientService::handleChatReply(const QString& intent, const QString& reply){
    qDebug() << "ClientService: " << intent << " " << reply;
    emit chatReplyString(reply);
    if(intent == "check_balance"){
        // TODO
    }else if(intent == "request_loan_recommendation"){
        // TODO: get real id
        recommendLoanAmount(21); // using a random id to test
    }
    else if(intent == "list_transactions"){
        // TODO
    }
    else if(intent == "greeting"){
        // don't have to do anything here...
    }
    else if(intent == "unknown"){
        // don't have to do anything here...
    }else{}
}
