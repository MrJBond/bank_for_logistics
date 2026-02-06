#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    try{
        m_db = DbConnector::getInstance();
        m_session = UserSession::getInstance();
    }catch(const std::runtime_error& e){
        qDebug() << e.what();
    }
    if(m_db){
        buildLoginDialog();
        connectLoginDialog();
        // services
        m_client_service = std::make_unique<ClientService>();
        m_account_service = std::make_unique<AccountService>();
        m_transaction_service = std::make_unique<TransactionService>();
        m_loan_service = std::make_unique<LoanService>();
        m_pythonServerProcess = new QProcess(this);
        startPythonServer();

        connect(m_client_service.get(), &ClientService::faceLoginSuccessful,
                this, [this](){login([](){});}); // to close the log in dialog

        if(m_dlgLogin.exec() != QDialog::Accepted){
            stopPythonServer();
            throw std::runtime_error("Failed to login!");
        }
        connect(m_client_service.get(), &ClientService::chatReplyString,
                this, &MainWindow::handleChatBotReply);
        connect(m_client_service.get(), &ClientService::finalLoanAmount,
                this, &MainWindow::handleFinalLoanAmount);
        connect(m_client_service.get(), &ClientService::loanRejection,
                this, &MainWindow::handleLoanRejection);
        connect(m_client_service.get(), &ClientService::networkFailure,
                this, &MainWindow::handleNetworkFailure);
        connect(m_client_service.get(), &ClientService::balanceCheckResult,
                this, &MainWindow::handleBalanceCheckResult);
        connect(m_client_service.get(), &ClientService::transactionListResult,
                this, &MainWindow::handleTransactionListResult);
        connect(m_loan_service.get(), &LoanService::loanResult,
                this, &MainWindow::handleLoanTakingResult);
        connect(m_transaction_service.get(), &TransactionService::createMessageBox,
                this, &MainWindow::onCreateMessageBox);
    }else{
        throw std::runtime_error("Failed to connect to db!");
    }
    this->setWindowIcon(QIcon(QPixmap(":/img/img/dollar.png")));
    this->setWindowTitle("Bank");
    ui->console->setTextColor(QColor(255,0,0));

    QString style = "background-color: white; color: red;";
    ui->menubar->setStyleSheet(style);
    qDebug() << " User session: " << m_session->getUserId()
             << " " << m_session->getUsername() << " "
             << m_session->isLoggedIn();
}

MainWindow::~MainWindow()
{
    stopPythonServer();
    m_session->destroySession();
    delete ui;
}
/***************************************************************
                    Log in / Sign up
 ***************************************************************/
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
    m_loginWithFaceButton = new QPushButton("Log in With Face", &m_dlgLogin);
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
    m_layoutLogin->addWidget(m_loginWithFaceButton);
    m_layoutLogin->addWidget(m_signupButton);
}
void MainWindow::resetLoginDialog(){
    if(m_layoutLogin){
        delete m_layoutLogin;
        m_layoutLogin = nullptr;
    }
    if(m_usernameInput){
        delete m_usernameInput;
        m_usernameInput = nullptr;
    }
    if(m_userpasswordInput){
        delete m_userpasswordInput;
        m_userpasswordInput = nullptr;
    }
    if(m_loginButton){
        delete m_loginButton;
        m_loginButton = nullptr;
    }
    if(m_loginWithFaceButton){
        delete m_loginWithFaceButton;
        m_loginWithFaceButton = nullptr;
    }
    if(m_signupButton){
        delete m_signupButton;
        m_signupButton = nullptr;
    }
    if(m_userRadioButton){
        delete m_userRadioButton;
        m_userRadioButton = nullptr;
    }
    if(m_dbUserRadioButton){
        delete m_dbUserRadioButton;
        m_dbUserRadioButton = nullptr;
    }
    if(m_usernameLabel){
        delete m_usernameLabel;
        m_usernameLabel = nullptr;
    }
    if(m_userpasswordLabel){
        delete m_userpasswordLabel;
        m_userpasswordLabel = nullptr;
    }
}
void MainWindow::disconnectLoginSignals(){
    disconnect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginDbUser);
    disconnect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginBankUser);
}
void MainWindow::connectLoginDialog(){
    connect(m_loginButton, &QPushButton::clicked, this, &MainWindow::attemptLoginDbUser);
    connect(m_loginWithFaceButton, &QPushButton::clicked, this, &MainWindow::onLoginWithFaceClicked);
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
void MainWindow::createUserSession(const int id, const QString& name){
    m_session->createSession(id, name);
}
void MainWindow::attemptLoginDbUser(){
    login([&](){
        m_db->reConnect(m_usernameInput->text(),
                        m_userpasswordInput->text()); // might throw and will be caught in login(...)
        m_session->destroySession();
    });
    // don't create a session here
}
void MainWindow::attemptLoginBankUser(){
    auto userLogin = [&](){
        qDebug() << "Bank user log in";
        const QString name = m_usernameInput->text();
        const QString password = m_userpasswordInput->text();
        const int id = m_session->login(name, password); // might throw std::runtime_error, it will be caught in login(...)
        createUserSession(id, name);
    };
    login(userLogin);
}
void MainWindow::loginWithFace(){
    FaceCaptureDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString base64Image = dlg.getCapturedImageBase64();
        qDebug() << "Image before requesting the face vector: " << base64Image.left(200);
        m_client_service->requestFaceVector(base64Image);
    }
}
void MainWindow::attemptSignupBankUser(){
    qDebug() << "Bank user sign up";
    auto userLogin = [&](){
        std::vector<QString> names = {"Client's name", "Address", "Name of the Boss",
                                  "Phone of the Boss", "Accountant's name", "Accountant's phone",
                                  "Password", "Confirm password"};
        std::unordered_map<QString, QString> res;
        while(1){
            res = createDialogInsert("Password", names);
            if(res["Password"] != res["Confirm password"])
                createMessageBox("There is a mismatch in passwords!");
            else
                break;
        }
        const int id = m_session->saveUserPassword(m_client_service.get(),
                                                   res["Client's name"], res["Password"], res["Address"],
                                                   res["Name of the Boss"], res["Phone of the Boss"],
                                                   res["Accountant's name"], res["Accountant's phone"]
                                                   ); // might throw std::runtime_error, it will be caught in login(...)
        createUserSession(id, res["Client's name"]);

        // face detection
        QDialog dlg(this);
        QFormLayout *layout = new QFormLayout();
        createDialogBox("Would you like to save your face to be able to log in by scanning it?", dlg, layout);
        if(dlg.exec() == QDialog::Accepted) {
            loginWithFace();
        }
    };
    login(userLogin);
}
void MainWindow::onLoginWithFaceClicked(){
    loginWithFace();
}
void MainWindow::onCreateMessageBox(const char* m){
    createMessageBox(m);
}
/***************************************************************
                    UI helper functions
 ***************************************************************/
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
std::unordered_map<QString, QString> MainWindow::createDialogInsert(QString title, std::vector<QString> names, AbstractService* service, QList<ComboData>* boxes){
    std::unordered_map<QString, QString> res;
    QDialog dlg(this);
    QFormLayout *layout = new QFormLayout();
    createDialogBox(title, dlg, layout);

    std::vector<QLineEdit*> lineEdits = createLineEdits(names, dlg, layout);
    if(service)
        showTable(dlg, layout, service);
    QList<QComboBox*> boxRes;
    if(boxes)
        boxRes = createComboBoxes(layout, *boxes);
    if(dlg.exec() == QDialog::Accepted) {
        for(size_t i = 0; i<lineEdits.size(); ++i){
            res[names[i]] = lineEdits[i]->text();
        }
        if(boxes){
            for(int i = 0; i < boxes->size(); ++i){
                const QString label = (*boxes)[i].label;
                const QString text = boxRes[i]->currentText();
                res[label] = text;
            }
        }
    }
    return res;
}
QTableWidget* MainWindow::buildTable(QDialog& dlg, const QString& title, AbstractService* service){
     dlg.resize(900, 700);
     QFormLayout *layout = new QFormLayout();
     createDialogBox(title, dlg, layout);
    QTableWidget *table = new QTableWidget(&dlg);
    layout->addWidget(table);
    try{
        service->getAll(ui->console, table);
        return table;
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    return nullptr;
}
void MainWindow::showTable(QDialog& dlg, QFormLayout *layout, AbstractService* service){
    QTableWidget *table = new QTableWidget(&dlg);
    layout->addWidget(table);
    try{
        service->getAll(ui->console, table);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    if(table){
        const int rows = table->rowCount();
        const int columns = table->columnCount();
        for(int i = 0; i<rows; ++i)
            for(int j = 0; j<columns; ++j)
                table->item(i,j)->setFlags(table->item(i,j)->flags() & ~Qt::ItemIsEditable);
    }
}
QList<QComboBox*> MainWindow::createComboBoxes(QFormLayout *layout, const QList<ComboData> &boxes) {
    QList<QComboBox*> createdWidgets;
    for(const auto& data : boxes){
        QComboBox *box = new QComboBox();
        box->addItems(data.options);
        layout->addRow(data.label, box);
        createdWidgets.append(box);
    }
    return createdWidgets;
}
QStringList MainWindow::getAccountsForUser(){
    const int id = m_session->getUserId();
    QStringList ids;
        // throws
    if(id != 0 && m_client_service->isClientPresent(id)){
        auto accs = m_client_service->getAccountsForClient(id);
        for(const auto& a : accs)
            ids.push_back(QString::number(a.getId()));
    }
    return ids;
}
/***********************************************
                    SHOW
 *************************************************/
void MainWindow::showHelper(AbstractService* service){
    ui->console->clear();
    try{
        service->getAll(ui->console);
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionShowClients_triggered()
{
    showHelper(m_client_service.get());
}
void MainWindow::on_actionShowAccounts_triggered()
{
    showHelper(m_account_service.get());
}
void MainWindow::on_actionShowTransactions_triggered()
{
    showHelper(m_transaction_service.get());
}
void MainWindow::on_actionShowLoans_triggered()
{
    showHelper(m_loan_service.get());
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
    std::unordered_map<QString, QString> dlg = createDialogInsert("Add loan amount", names,  m_account_service.get());
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
void MainWindow::on_actionMake_transaction_triggered(){
    std::vector<QString> names = {"Destination Account Id", "Amount", "Description"};
    QList<ComboData> comboData;
    comboData.push_back({"Source Account Id", getAccountsForUser()});
    std::unordered_map<QString, QString> dlg = createDialogInsert("Make a transaction",
                                                                  names,  m_account_service.get(), &comboData);
    if(dlg.size() != 0){
        try{
            m_transaction_service->requestTransaction(dlg["Source Account Id"].toInt(),
                                                   dlg["Destination Account Id"].toInt(),
                                                   dlg["Amount"].toDouble(), dlg["Description"]);
        }catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }catch(...){
            createMessageBox("Transaction failed!");
        }
    }
}
void MainWindow::on_actionTake_loan_triggered(){
    try{
        std::vector<QString> names = {"Amount"};
        QList<ComboData> comboData;
        comboData.push_back({"Account Id", getAccountsForUser()});
        comboData.push_back({"Duration (Months)", {"12", "24", "48", "60", "120"}});
        std::unordered_map<QString, QString> res = createDialogInsert("Take loan", names, nullptr, &comboData);
        if(res.size() != 0)
            m_loan_service->requestLoan(res["Account Id"].toInt(),
                                        res["Amount"].toDouble(),
                                        res["Duration (Months)"].toInt());
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }catch(...){
        createMessageBox("Loan operation failed!");
    }
}

QSortFilterProxyModel* MainWindow::setTabWidget(AbstractService* service, QTabWidget *tabWidget, const QString& name){
    QTableWidget *table = new QTableWidget();
    try{
        service->getAll(nullptr, table);
    }
    catch(const std::exception& e)
    {
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    QTableView* tableView = new QTableView();
    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
    proxyModel->setSourceModel(table->model());
    // Set filter to be case-insensitive (A == a)
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // Tell it to search all columns (or specific one, e.g., Description is column 5)
    proxyModel->setFilterKeyColumn(-1); // -1 means search all columns
    tableView->setModel(proxyModel);
    const int n = tabWidget->addTab(tableView, name);
    Q_UNUSED(n);
    return proxyModel;
}
void MainWindow::on_actionSearch_triggered(){
    QDialog dlg(this);
    QFormLayout * layout = new QFormLayout();
    createDialogBox("Search", dlg, layout);
    std::vector<QLineEdit*> lineEdit = createLineEdits({"Search"}, dlg, layout);
    QTabWidget *tabWidget = new QTabWidget();
    QSortFilterProxyModel* a = setTabWidget(m_account_service.get(), tabWidget, "Accounts");
    QSortFilterProxyModel* c = setTabWidget(m_client_service.get(), tabWidget, "Clients");
    QSortFilterProxyModel* l = setTabWidget(m_loan_service.get(), tabWidget, "Loans");
    QSortFilterProxyModel* t = setTabWidget(m_transaction_service.get(), tabWidget, "Transactions");
    connect(lineEdit[0], &QLineEdit::textChanged, this, [&](const QString& arg1){
        // This creates a regex so "Food" matches "food", "Seafood", etc.
        QRegularExpression regex(arg1, QRegularExpression::CaseInsensitiveOption);
        a->setFilterRegularExpression(regex);
        c->setFilterRegularExpression(regex);
        l->setFilterRegularExpression(regex);
        t->setFilterRegularExpression(regex);
    });
    layout->addWidget(tabWidget);
    dlg.resize(this->width(), this->height());
    dlg.exec();
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
std::pair<int, QString> MainWindow::insertClient(){
    std::vector<QString> names = {"Client's name", "Address", "Name of the Boss",
                                  "Phone of the Boss", "Accountant's name", "Accountant's phone"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the client", names);
    if(res.size() != 0){
        int id = 0;
        try{
            id = m_client_service->insertClient(res["Client's name"],
                                                res["Address"],
                                                res["Name of the Boss"],
                                                res["Phone of the Boss"],
                                                res["Accountant's name"],
                                                res["Accountant's phone"]);
            return std::make_pair(id, res["Client's name"]);
        }catch(const std::invalid_argument& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
        catch(const std::runtime_error& e){
            createMessageBox(e.what());
            qDebug() << e.what();
        }
    }
    return std::make_pair(0, res["Client's name"]);
}
void MainWindow::on_actionInsert_Client_triggered(){
    std::pair<int, QString> c = insertClient();
    qDebug() << c.first << " " << c.second;
}
void MainWindow::on_actionInsert_Account_triggered(){
    std::vector<QString> names = {"Amount", "Currency", "Client's Id"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the account", names);
    if(res.size() != 0){
        int id = 0;
        try{
            id = m_account_service->insertAccount(res["Client's Id"].toInt(),
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
    std::vector<QString> names = {"Date (yyyy-MM-dd)", "Amount", "Source Account", "Destination Account", "Description"};
    std::unordered_map<QString, QString> res = createDialogInsert("Insert the transaction", names);
    if(res.size() != 0){
        int id = 0;
        try{
            id = m_transaction_service->insertTransaction(QDate::fromString(res["Date (yyyy-MM-dd)"], "yyyy-MM-dd"),
                                                     res["Amount"].toDouble(),
                                                     res["Source Account"].toInt(),
                                                     res["Destination Account"].toInt(),
                                                     res["Description"]);
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
        int id = 0;
        try{
            id = m_loan_service->insertLoan(res["Account"].toInt(),
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
void MainWindow::updateHelper(const QString& title,
                  AbstractService* service,
                  std::function<void(QVector<QString> data)> update){
    QDialog dlg(this);
    QTableWidget *table = buildTable(dlg, title, service);
     if(table){
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
                                             object[4].toInt(),
                                             object[5]);
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
                 m_client_service.get(),
                 [&](QVector<QString> object){updateClient(object);});
}
void MainWindow::on_actionUpdate_Account_triggered(){
    updateHelper("Update Account",
                 m_account_service.get(),
                 [&](QVector<QString> object){updateAccount(object);});
}
void MainWindow::on_actionUpdate_Transaction_triggered(){
    updateHelper("Update Transaction",
                 m_transaction_service.get(),
                 [&](QVector<QString> object){updateTransaction(object);});
}
void MainWindow::on_actionUpdate_Loan_triggered(){
    updateHelper("Update Loan",
                 m_loan_service.get(),
                 [&](QVector<QString> object){updateLoan(object);});
}
/*************************************************************
                        DELETE
**************************************************************/
void MainWindow::deleteHelper(QString table_name, AbstractService* service){
    QDialog dlg(this);
    QFormLayout *layout = new QFormLayout();
    createDialogBox("Delete " + table_name, dlg, layout);

    std::vector<QString> names = {"Delete (id)"};
    std::vector<QLineEdit*> lineEdits = createLineEdits(names, dlg, layout);
    showTable(dlg, layout, service);
    if(dlg.exec() == QDialog::Accepted) {
        try{
            service->deleteObj(lineEdits[0]->text().toInt());
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
    deleteHelper("Client", m_client_service.get());
}
void MainWindow::on_actionDelete_Account_triggered(){
    deleteHelper("Account", m_account_service.get());
}
void MainWindow::on_actionDelete_Transaction_triggered(){
    deleteHelper("Transaction", m_transaction_service.get());
}
void MainWindow::on_actionDelete_Loan_triggered(){
    deleteHelper("Loan", m_loan_service.get());
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
void MainWindow::on_actionLog_out_triggered(){
    m_session->destroySession();
    qDebug() << m_session->getUserId() << " " << m_session->getUsername();
}
void MainWindow::on_actionReset_triggered(){
    resetLoginDialog();
    buildLoginDialog();
    connectLoginDialog();
    m_loginWithFaceButton->setEnabled(false); /* disable to
    prevent a wrong face from being saved with the current user
    in ClientService::handleUserFaceVector
    */
    if(m_dlgLogin.exec() != QDialog::Accepted)
       qDebug() << "Failed to login!";
    else{
        if(m_userRadioButton->isChecked()){
            try{ // set the default user for the bank Client
                m_db->reConnect("bank_app_user",
                                    "qwerty");
            }catch(const std::runtime_error& e){
                qDebug() << e.what();
            }
        }else{ // db user
            try{
                m_db->reConnect(m_usernameInput->text(),
                                    m_userpasswordInput->text());
            }catch(const std::runtime_error& e){
                qDebug() << e.what();
            }
        }
    }
    qDebug() << m_session->getUserId() << " " << m_session->getUsername();
}
void MainWindow::on_actionWho_am_I_triggered(){
    QString str = QString::number(m_session->getUserId()) +
                  " " + m_session->getUsername() + " " +
                  (m_session->isLoggedIn() ? "true" : "false");
    createMessageBox(str.toStdString().c_str());
}
/**********************************************************
                        CHARTS
 **********************************************************/
void MainWindow::on_actionTransactionsChart_triggered(){
    m_transaction_service->buildTransactionsChart(this->width(), this->height());
}
void MainWindow::on_actionTotal_currencyChart_triggered(){
    m_account_service->currencyChart(this->width(), this->height());
}
void MainWindow::on_actionIncome_Expenses_triggered(){
    try{
        m_client_service->incomeExpensesChart(this->width(), this->height());
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionBalance_History_triggered(){
    try{
        m_client_service->balanceHistoryChart(this->width(), this->height());
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        createMessageBox(e.what());
        qDebug() << e.what();
    }
}
void MainWindow::on_actionSpendingChart_triggered(){
    m_transaction_service->buildSpendingPieChart(this->width(), this->height());
}
/****************************************************
     *                      AI Lab2
****************************************************/
void MainWindow::on_actionRecommend_loan_amount_triggered(){
    try{
        m_client_service->recommendLoanAmount(m_session->getUserId()); // Client id = 21 is good for testing
    }catch(const std::runtime_error& e){
        createMessageBox(e.what());
    }
}
void MainWindow::startPythonServer()
{
    QString pythonExecutable = "F:/Python/python.exe";
    QString scriptPath = "../../AI/loan_recommender.py";

    // Set up the arguments
    QStringList args;
    args << "-u" << scriptPath; // unbuffered


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
    connect(&dlg, &QDialog::finished, this, [&](int result) {
        if(m_chatHistory){
            delete m_chatHistory;
            m_chatHistory = nullptr;
        }
    });
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
                m_client_service->handleUserMessage(newText);
            }
        });
    }
    dlg.exec();
}
void MainWindow::handleChatBotReply(const QString& reply){
    if(m_chatHistory)
        m_chatHistory->append("<b>Penny: </b> " + reply);
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
void MainWindow::handleBalanceCheckResult(const std::vector<Account>& accounts){
    ui->console->clear();
    for(const Account& a : accounts)
        ui->console << a;
}
void MainWindow::handleTransactionListResult(const std::vector<Transaction>& transactions){
    ui->console->clear();
    for(const Transaction& t : transactions)
        ui->console << t;
}
void MainWindow::handleLoanTakingResult(bool approved, const QString& msg){
    createMessageBox(msg.toStdString().c_str());
}
