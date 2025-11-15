#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    try{
        m_db = DbConnector::getInstance();
    }catch(const std::runtime_error& e){
        qDebug() << e.what();
    }
    if(m_db){
        buildLoginDialog();
        auto disconnectLoginSignals = [&](){
            disconnect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginDbUser);
            disconnect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginBankUser);
        };
        connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginDbUser);
        m_signupButton->setEnabled(false);
        connect(m_dbUserRadioButton, &QRadioButton::toggled, this, [&](){
            m_signupButton->setEnabled(false);
            disconnectLoginSignals();
            connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginDbUser);
        });
        connect(m_userRadioButton, &QRadioButton::toggled, this, [&](){
            m_signupButton->setEnabled(true);
            disconnectLoginSignals();
            connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginBankUser);
        });
        connect(m_signupButton, &QPushButton::clicked, this, &MainWindow::attemptSignupBankUser);
        if(m_dlgLogin.exec() != QDialog::Accepted){
            throw std::runtime_error("Failed to login!");
        }
        // services
        m_client_service = std::make_unique<ClientService>();
        m_account_service = std::make_unique<AccountService>();
        m_transaction_service = std::make_unique<TransactionService>();
        m_loan_service = std::make_unique<LoanService>();
        m_pythonServerProcess = new QProcess(this);
        startPythonServer();
        connect(m_client_service.get(), &ClientService::chatReplyString,
                this, &MainWindow::handleChatBotReply);
        connect(m_client_service.get(), &ClientService::finalLoanAmount,
                this, &MainWindow::handleFinalLoanAmount);
        connect(m_client_service.get(), &ClientService::loanRejection,
                this, &MainWindow::handleLoanRejection);
        connect(m_client_service.get(), &ClientService::networkFailure,
                this, &MainWindow::handleNetworkFailure);
    }else{
        throw std::runtime_error("Failed to connect to db!");
    }
    this->setWindowIcon(QIcon(QPixmap(":/img/img/dollar.png")));
    this->setWindowTitle("Bank");
    ui->console->setTextColor(QColor(255,0,0));

    QString style = "background-color: white; color: red;";
    ui->menubar->setStyleSheet(style);
}

MainWindow::~MainWindow()
{
    stopPythonServer();
    delete ui;
}
void MainWindow::buildLoginDialog(){
    m_dlgLogin.setModal(true);
    m_dlgLogin.setWindowTitle("Login");
    m_dlgLogin.setWindowIcon(QIcon(QPixmap(":/img/img/dollar.png")));
    m_layoutLogin = new QVBoxLayout(&m_dlgLogin);
    m_usernameLabel = new QLabel("Username:", &m_dlgLogin);
    m_userpasswordLabel = new QLabel("Password:", &m_dlgLogin);
    m_usernameInput = new QLineEdit(&m_dlgLogin);
    m_userpasswordInput = new QLineEdit(&m_dlgLogin);
    m_userpasswordInput->setEchoMode(QLineEdit::Password);
    m_loginButton = new QPushButton("Log in", &m_dlgLogin);
    m_signupButton = new QPushButton("Sign up", &m_dlgLogin);
    m_signupButton->setEnabled(false);
    m_userRadioButton = new QRadioButton("Login as a bank user");
    m_dbUserRadioButton = new QRadioButton("Login as a DB user");
    m_dbUserRadioButton->setChecked(true);

    m_layoutLogin->addWidget(m_userRadioButton);
    m_layoutLogin->addWidget(m_dbUserRadioButton);
    m_layoutLogin->addWidget(m_usernameLabel);
    m_layoutLogin->addWidget(m_usernameInput);
    m_layoutLogin->addWidget(m_userpasswordLabel);
    m_layoutLogin->addWidget(m_userpasswordInput);
    m_layoutLogin->addWidget(m_loginButton);
    m_layoutLogin->addWidget(m_signupButton);
}
void MainWindow::login(std::function<void()> loginUser){
    if(m_db){
        try{
            loginUser();
            m_dlgLogin.accept();
        }catch(const std::runtime_error& e){
            m_usernameInput->clear();
            m_userpasswordInput->clear();
            createMessageBox(e.what());
        }
    }
}
void MainWindow::attemptLoginDbUser(){
    login([&](){
        m_db->reConnect(m_usernameInput->text(),
                        m_userpasswordInput->text());
    });
}
void MainWindow::attemptLoginBankUser(){
    auto userLogin = [&](){
        qDebug() << "Bank user log in";
        const QString name = m_usernameInput->text();
        const QString password = m_userpasswordInput->text();
        /*
         * TODO:
         * 1. id = m_clientRepo->getClient(name, password)
         *    if must throw std::runtime_error
         * 2. set current user
         */
        throw std::runtime_error("Check if user exists ");
    };
    login(userLogin);
}
void MainWindow::attemptSignupBankUser(){
    qDebug() << "Bank user sign up";
    on_actionInsert_Client_triggered();
    /* TODO:
     * 1. Create a separate insert function
     * 2. get id from insert
     * 3. log in

    */
}
void MainWindow::createDialogBox(QString title, QDialog& dlg, QFormLayout *layout){
    dlg.setWindowTitle(title);
    QDialogButtonBox *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btn_box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btn_box);
    dlg.setLayout(layout);
}

std::vector<QLineEdit*> MainWindow::createLineEdits(std::vector<QString> names,
                                                     QDialog& dlg,
                                                     QFormLayout *layout){
    std::vector<QLineEdit*> edits;
    for(const QString& name : names){
        QLineEdit *ledit = new QLineEdit(&dlg);
        layout->addRow(tr(name.toStdString().c_str()), ledit);
        edits.push_back(ledit);
    }
    return edits;
}
std::unordered_map<QString, QString> MainWindow::createDialogInsert(QString title, std::vector<QString> names){
    std::unordered_map<QString, QString> res;
    QDialog dlg(this);
    QFormLayout *layout = new QFormLayout();
    createDialogBox(title, dlg, layout);

    std::vector<QLineEdit*> lineEdits = createLineEdits(names, dlg, layout);
    if(dlg.exec() == QDialog::Accepted) {
        for(size_t i = 0; i<lineEdits.size(); ++i){
            res[names[i]] = lineEdits[i]->text();
        }
    }
    return res;
}
/***********************************************
                    SHOW
 *************************************************/
void MainWindow::on_actionShowClients_triggered()
{
    ui->console->clear();
    try{
        m_client_service->getAllClients(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}


void MainWindow::on_actionShowAccounts_triggered()
{
    ui->console->clear();
    try{
        m_account_service->getAllAccounts(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}


void MainWindow::on_actionShowTransactions_triggered()
{
    ui->console->clear();
    try{
        m_transaction_service->getAllTransactions(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}


void MainWindow::on_actionShowLoans_triggered()
{
    ui->console->clear();
    try{
        m_loan_service->getAllLoans(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
/***********************************************
                    FUNCTIONS
 *************************************************/
void MainWindow::on_actionGet_clients_with_total_transaction_sum_triggered(){
    ui->console->clear();
    try{
        m_client_service->getClientsWithTotalSum(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionGets_clients_legal_and_individuals_sorted_by_amount_of_account_balances_triggered(){
    ui->console->clear();
    try{
        m_client_service->getClientsLegalIndividual(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionGet_legal_clients_with_total_transaction_sum_triggered(){
    ui->console->clear();
    try{
        m_client_service->getLegalClientsWithTotalSum(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionGet_total_earned_money_triggered(){
    ui->console->clear();
    try{
        m_loan_service->getTotalEarnedMoney(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionGet_individual_loans_triggered(){
    ui->console->clear();
    try{
        m_loan_service->getIndividualLoans(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionGet_average_legal_loans_triggered(){
    ui->console->clear();
    try{
        m_loan_service->getAverageLegalLoans(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionUpdate_account_amounts_depending_on_the_transactions_triggered(){
    try{
        m_account_service->updateAmountOnTransactions();
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionAdd_the_amount_loan_to_the_account_triggered(){
    std::vector<QString> names = {"Id Account", "Loan Amount"};
    std::unordered_map<QString, QString> dlg = createDialogInsert("Add loan amount", names);
    if(dlg.size() != 0)
    try{
        m_account_service->addLoanToAccount(dlg["Id Account"].toInt(), dlg["Loan Amount"].toDouble());
    }
    catch(const std::runtime_error& e){
       createMessageBox(e.what());
       qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
/***********************************************
                    REPORTS
 *************************************************/
void MainWindow::on_actionLoanReport_triggered()
{
    bool ok = true;
    try{
        m_loan_service->setFileName("loanReport.xml");
    }catch(const std::invalid_argument& e){
        ok = false;
        e.what();
    }
    if(ok){
        try{
            m_loan_service->loanReportWithAllLoans(this);
        }catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionClientsWithTotalSumReport_triggered(){
    bool ok = true;
    try{
        m_client_service->setFileName("clientTransaction.xml");
    }catch(const std::invalid_argument& e){
        ok = false;
        e.what();
    }
    if(ok){
        try{
            m_client_service->getClientsWithTotalSumReport(this);
        }catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionClientAccountReport_triggered(){
    bool ok = true;
    try{
        m_client_service->setFileName("withNested.xml");
    }catch(const std::invalid_argument& e){
        ok = false;
        e.what();
    }
    if(ok){
        try{
            m_client_service->getClientsAccountsReport(this);
        }catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::createMessageBox(const char* message){
    QMessageBox box;
    box.setText(message);
    box.setIcon(QMessageBox::Warning);
    box.exec();
}
/*************************************************************
                        INSERT
**************************************************************/
void MainWindow::on_actionInsert_Client_triggered(){
    std::vector<QString> names = {"Client's name", "Address", "Name of the Boss",
    "Phone of the Boss", "Accountant's name", "Accountant's phone"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the client", names);
    if(res.size() != 0){
        try{
            m_client_service->insertClient(res["Client's name"],
                                           res["Address"],
                                           res["Name of the Boss"],
                                           res["Phone of the Boss"],
                                           res["Accountant's name"],
                                           res["Accountant's phone"]);
        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionInsert_Account_triggered(){
    std::vector<QString> names = {"Amount", "Currency", "Client's Id"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the account", names);
    if(res.size() != 0){
        try{
            m_account_service->insertAccount(res["Client's Id"].toInt(),
                                             res["Amount"].toDouble(),
                                             res["Currency"]);

        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionInsert_Transaction_triggered(){
    std::vector<QString> names = {"Date (yyyy-MM-dd)", "Amount", "Source Account", "Destination Account"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the transaction", names);
    if(res.size() != 0){
        try{
            m_transaction_service->insertTransaction(QDate::fromString(res["Date (yyyy-MM-dd)"], "yyyy-MM-dd"),
                                                     res["Amount"].toDouble(),
                                                     res["Source Account"].toInt(),
                                                     res["Destination Account"].toInt());
        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionInsert_Loan_triggered(){
    std::vector<QString> names = {"Account", "Issue Date (yyyy-MM-dd)",
                                  "Usage Date (yyyy-MM-dd)", "Percent","Amount"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the loan", names);
    if(res.size() != 0){
        try{
            m_loan_service->insertLoan(res["Account"].toInt(),
                                       QDate::fromString(res["Issue Date (yyyy-MM-dd)"], "yyyy-MM-dd"),
                                       QDate::fromString(res["Usage Date (yyyy-MM-dd)"], "yyyy-MM-dd"),
                                       res["Percent"].toDouble(),
                                       res["Amount"].toDouble());
        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
/*************************************************************
                        UPDATE
**************************************************************/
void MainWindow::updateHelper(QString title,
                  std::function<void(QTextBrowser* browser, QTableWidget *table)> getAll,
                  std::function<void(QVector<QString> data)> update){
    QDialog dlg(this);
    dlg.resize(900, 700);
    QFormLayout *layout = new QFormLayout();
    createDialogBox(title, dlg, layout);
    QTableWidget *table = new QTableWidget(&dlg);
    layout->addWidget(table);
    try{
        getAll(ui->console, table);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    // work with table
    if(dlg.exec() == QDialog::Accepted) {
        int col = table->columnCount();
        size_t row = table->rowCount();
        for(size_t i = 0; i<row; ++i){
            QVector<QString> object;
            for(int j = 0; j<col; ++j){
                object.push_back(table->item(i,j)->text());
            }
            try{
                update(object);
            }catch(const std::invalid_argument& e){
                createMessageBox(e.what());
                qDebug() << e.what();
            }
            catch(const std::runtime_error& e){
                createMessageBox(e.what());
                qDebug() << e.what();
            }
        }
    }
}
void MainWindow::updateClient(QVector<QString> object){
    // this may throw (will be caught in updateHelper)
    m_client_service->updateClient(object[0].toInt(),
                                   object[1],
                                   object[2],
                                   object[3],
                                   object[4],
                                   object[5],
                                   object[6]);
}
void MainWindow::updateAccount(QVector<QString> object){
    // this may throw (will be caught in updateHelper)
    m_account_service->updateAccount(object[0].toInt(),
                                     object[3].toInt(),
                                     object[1].toDouble(),
                                     object[2]);
}
void MainWindow::updateTransaction(QVector<QString> object){
    // this may throw (will be caught in updateHelper)
    m_transaction_service->updateTransaction(object[0].toInt(),
                                             QDate::fromString(object[1], "ddd MMM d yyyy"),
                                             object[2].toDouble(),
                                             object[3].toInt(),
                                             object[4].toInt());
}
void MainWindow::updateLoan(QVector<QString> object){
    // this may throw (will be caught in updateHelper)
    m_loan_service->updateLoan(object[0].toInt(),
                               object[1].toInt(),
                               QDate::fromString(object[2], "ddd MMM d yyyy"),
                               QDate::fromString(object[3], "ddd MMM d yyyy"),
                               object[4].toDouble(),
                               object[5].toDouble());
}
void MainWindow::on_actionUpdate_Client_triggered(){
    updateHelper("Update Client",
                 [&](QTextBrowser* browser, QTableWidget *table){m_client_service->getAllClients(browser, table);},
                 [&](QVector<QString> object){updateClient(object);});
}
void MainWindow::on_actionUpdate_Account_triggered(){
    updateHelper("Update Account",
                 [&](QTextBrowser* browser, QTableWidget *table){m_account_service->getAllAccounts(browser, table);},
                 [&](QVector<QString> object){updateAccount(object);});
}
void MainWindow::on_actionUpdate_Transaction_triggered(){
    updateHelper("Update Transaction",
                 [&](QTextBrowser* browser, QTableWidget *table){m_transaction_service->getAllTransactions(browser, table);},
                 [&](QVector<QString> object){updateTransaction(object);});
}
void MainWindow::on_actionUpdate_Loan_triggered(){
    updateHelper("Update Loan",
                 [&](QTextBrowser* browser, QTableWidget *table){m_loan_service->getAllLoans(browser, table);},
                 [&](QVector<QString> object){updateLoan(object);});
}
/*************************************************************
                        DELETE
**************************************************************/
void MainWindow::deleteHelper(QString table_name, std::function<void(int)> del){
    QDialog dlg(this);
    QFormLayout *layout = new QFormLayout();
    createDialogBox("Delete " + table_name, dlg, layout);

    std::vector<QString> names = {"Delete (id)"};
    std::vector<QLineEdit*> lineEdits = createLineEdits(names, dlg, layout);
    if(dlg.exec() == QDialog::Accepted) {
        try{
            del(lineEdits[0]->text().toInt());
        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
}
void MainWindow::on_actionDelete_Client_triggered(){
    deleteHelper("Client", [&](int id){m_client_service->deleteClient(id);});
}
void MainWindow::on_actionDelete_Account_triggered(){
    deleteHelper("Account", [&](int id){m_account_service->deleteAccount(id);});
}
void MainWindow::on_actionDelete_Transaction_triggered(){
    deleteHelper("Transaction", [&](int id){m_transaction_service->deleteTransaction(id);});
}
void MainWindow::on_actionDelete_Loan_triggered(){
    deleteHelper("Loan", [&](int id){m_loan_service->deleteLoan(id);});
}
/*************************************************************
                        VIEWS
**************************************************************/
void MainWindow::on_actionTransaction_view_triggered(){
    ui->console->clear();
    try{
        m_transaction_service->getTransactionView(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionClient_Loan_AccountView_triggered(){
    ui->console->clear();
    try{
        m_account_service->getClientLoanAccountView(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionDirectorView_triggered(){
    ui->console->clear();
    try{
        m_client_service->getDirectorView(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionLoan_ClientView_triggered(){
    ui->console->clear();
    try{
        m_loan_service->getLoanClientView(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionLoanView_triggered(){
    ui->console->clear();
    try{
        m_loan_service->getLoanView(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
/*************************************************************
                        USER
**************************************************************/
void MainWindow::on_actionReset_triggered(){
    std::vector<QString> names = {"User name ", "Password "};
    std::unordered_map<QString, QString> res = createDialogInsert("Reset user", names);
    if(res.size() != 0)
        try{
            m_db->reConnect(res[names[0]], res[names[1]]);
        }catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
}
/**********************************************************
                        CHARTS
 **********************************************************/
void MainWindow::on_actionChart_triggered(){
    /*
    TODO:
    1. Charts
    */
    m_transaction_service->buildTransactionsChart(300, 500);
}

/****************************************************
     *                      AI Lab2
****************************************************/
void MainWindow::on_actionRecommend_loan_amount_triggered(){
    m_client_service->recommendLoanAmount(21); // Client id = 21
    /* TODO:
      1. Get a client
     */
}
void MainWindow::startPythonServer()
{
    QString pythonExecutable = "F:/Python/python.exe";
    QString scriptPath = "../../AI/loan_recommender.py";

    // Set up the arguments
    QStringList args;
    args << scriptPath;


    // This connects to a slot that handles errors *during* the process lifetime
    connect(m_pythonServerProcess, &QProcess::errorOccurred, this, [&](QProcess::ProcessError error) {
        qDebug() << "QProcess Error:" << error;
        qDebug() << "QProcess Error String:" << m_pythonServerProcess->errorString();
    });

    // This connects to a slot that handles output from the process
    connect(m_pythonServerProcess, &QProcess::readyReadStandardError, this, [&]() {
        qDebug() << "Python Server stderr:" << m_pythonServerProcess->readAllStandardError();
    });

    connect(m_pythonServerProcess, &QProcess::readyReadStandardOutput, this, [&]() {
        qDebug() << "Python Server stdout:" << m_pythonServerProcess->readAllStandardOutput();
    });


    // Start the process
    m_pythonServerProcess->start(pythonExecutable, args);

    // Wait a moment to ensure it has started, with error checking
    if (!m_pythonServerProcess->waitForStarted(3000)) { // 3 second timeout
        qDebug() << "Error: Python server failed to start.";
        qDebug() << m_pythonServerProcess->errorString();
    } else {
        qDebug() << "Python server started successfully.";
    }
}

void MainWindow::stopPythonServer()
{
    qDebug() << "Stopping Python server...";
    if (m_pythonServerProcess->state() == QProcess::Running)
    {
        // 1. First, try to terminate gracefully
        m_pythonServerProcess->terminate();

        // 2. Wait up to 3 seconds for a graceful exit
        if (m_pythonServerProcess->waitForFinished(3000)) {
            qDebug() << "Python server stopped gracefully.";
            return; // Success!
        }

        // 3. Graceful exit failed. Force kill.
        qDebug() << "Server did not terminate gracefully. Forcing kill.";
        m_pythonServerProcess->kill();

        // 4. Wait again (up to 5 sec) for the OS to finish the kill.
        if (m_pythonServerProcess->waitForFinished(5000)) {
            qDebug() << "Python server stopped after kill.";
        } else {
            // This would be unusual, but good to know
            qDebug() << "Warning: Python server kill may have failed.";
        }
    }
    else
    {
        qDebug() << "Python server was not running.";
    }
}
/********************************************************
 *                         CHATBOT
 *********************************************************/
void MainWindow::on_actionChat_triggered(){
    m_chatHistory = new QTextEdit();
    QDialog dlg(this);
    QFormLayout *layout = new QFormLayout();
    createDialogBox("Chat", dlg, layout);
    m_chatHistory->setReadOnly(true);
    layout->addRow(m_chatHistory);
    std::vector<QString> names;
    names.push_back("Ask: ");
    std::vector<QLineEdit*> lineEdits = createLineEdits(names, dlg, layout);

    // 1. Find the button box and the "OK" button
    QDialogButtonBox *buttonBox = dlg.findChild<QDialogButtonBox*>();
    QPushButton *okButton = nullptr;

    if (buttonBox) {
        disconnect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        okButton = buttonBox->button(QDialogButtonBox::Ok);
    }

    if (okButton) {
        connect(okButton, &QPushButton::clicked, [lineEdits, this]() {
            // This code runs *instead* of closing the dialog
            QString newText = lineEdits[0]->text();
            if (!newText.isEmpty()) {
                m_chatHistory->append("<b>You: </b> " + newText); // Append new text
                lineEdits[0]->clear(); // Clear the input field
                m_client_service->getBotResponse(newText);
            }
        });
    }
    dlg.exec();
}
void MainWindow::handleChatBotReply(const QString& reply){
    m_chatHistory->append("<b>Bot: </b> " + reply);
}
void MainWindow::handleNetworkFailure(const QString& errorString){
    createMessageBox(errorString.toStdString().c_str());
}
void MainWindow::handleLoanRejection(){
    createMessageBox("Loan Rejected!");
}
void MainWindow::handleFinalLoanAmount(double amount){
    QString str("Recommended Loan Amount is ");
    str += QString::number(amount);
    createMessageBox(str.toStdString().c_str());
}
