#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include "qlineedit.h"
#include <windows.h>
#include "db/dbconnector.h"
#include "entities/client.h"
#include "service/accountservice.h"
#include "service/clientservice.h"
#include "service/loanservice.h"
#include "service/transactionservice.h"
#include "auth/usersession.h"
#include "CV/facecapturedialog.h"
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void attemptLoginDbUser();
    void attemptLoginBankUser();
    void attemptSignupBankUser();
    void loginWithFace();
    void onLoginWithFaceClicked();
    void onCreateMessageBox(const char* m);
    // show all
    void on_actionShowClients_triggered();

    void on_actionShowTransactions_triggered();

    void on_actionShowAccounts_triggered();

    void on_actionShowLoans_triggered();
    // functions
    void on_actionGet_clients_with_total_transaction_sum_triggered();
    void on_actionGets_clients_legal_and_individuals_sorted_by_amount_of_account_balances_triggered();
    void on_actionGet_legal_clients_with_total_transaction_sum_triggered();
    void on_actionGet_total_earned_money_triggered();
    void on_actionGet_individual_loans_triggered();
    void on_actionGet_average_legal_loans_triggered();
    void on_actionUpdate_account_amounts_depending_on_the_transactions_triggered();
    void on_actionAdd_the_amount_loan_to_the_account_triggered();
    void on_actionRecommend_loan_amount_triggered();
    void on_actionMake_transaction_triggered();
    void on_actionTake_loan_triggered();
    // reports
    void on_actionLoanReport_triggered();
    void on_actionClientsWithTotalSumReport_triggered();
    void on_actionClientAccountReport_triggered();
    // insert
    void on_actionInsert_Client_triggered();
    void on_actionInsert_Account_triggered();
    void on_actionInsert_Transaction_triggered();
    void on_actionInsert_Loan_triggered();
    // update
    void on_actionUpdate_Client_triggered();
    void on_actionUpdate_Account_triggered();
    void on_actionUpdate_Transaction_triggered();
    void on_actionUpdate_Loan_triggered();
    // delete
    void on_actionDelete_Client_triggered();
    void on_actionDelete_Account_triggered();
    void on_actionDelete_Transaction_triggered();
    void on_actionDelete_Loan_triggered();
    // views
    void on_actionTransaction_view_triggered();
    void on_actionClient_Loan_AccountView_triggered();
    void on_actionDirectorView_triggered();
    void on_actionLoan_ClientView_triggered();
    void on_actionLoanView_triggered();
    // user
    void on_actionReset_triggered();
    void on_actionLog_out_triggered();
    void on_actionWho_am_I_triggered();
    // charts
    void on_actionTransactionsChart_triggered();
    void on_actionTotal_currencyChart_triggered();
    void on_actionIncome_Expenses_triggered();
    void on_actionBalance_History_triggered();
    // Chat
    void on_actionChat_triggered();
    void handleChatBotReply(const QString& reply);
    void handleNetworkFailure(const QString& errorString);
    void handleLoanRejection();
    void handleFinalLoanAmount(double amount);
    void handleBalanceCheckResult(const std::vector<Account>& accounts);
    void handleTransactionListResult(const std::vector<Transaction>& transactions);
    void handleLoanTakingResult(bool approved, const QString& msg);
private:
    Ui::MainWindow *ui;
    DbConnector* m_db = nullptr;
    UserSession* m_session = nullptr;
    QDialog m_dlgLogin;
    QVBoxLayout *m_layoutLogin = nullptr;
    QLineEdit *m_usernameInput = nullptr;
    QLineEdit *m_userpasswordInput = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_loginWithFaceButton = nullptr;
    QPushButton *m_signupButton = nullptr;
    QRadioButton *m_userRadioButton = nullptr;
    QRadioButton *m_dbUserRadioButton = nullptr;
    QLabel *m_usernameLabel = nullptr;
    QLabel *m_userpasswordLabel = nullptr;

    void login(std::function<void()> loginUser);
    void createUserSession(const int id, const QString& name);
    void buildLoginDialog();
    void disconnectLoginSignals();
    void resetLoginDialog();
    void connectLoginDialog();
    void createMessageBox(const char* message);
    QTableWidget* buildTable(QDialog& dlg, const QString& title, AbstractService* service);
    void showTable(QDialog& dlg, QFormLayout *layout, AbstractService* service);
    std::vector<QLineEdit*> createLineEdits(std::vector<QString> names, QDialog& dlg, QFormLayout *layout);
    void createDialogBox(QString title, QDialog& dlg, QFormLayout *layout);
    struct ComboData {
        QString label;
        QStringList options;
    };
    QList<QComboBox*> createComboBoxes(QFormLayout *layout, const QList<ComboData> &boxes);
    QStringList getAccountsForUser();
    std::unordered_map<QString, QString> createDialogInsert(QString title, std::vector<QString> names, AbstractService* service = nullptr, QList<ComboData>* boxes = nullptr);
    void showHelper(AbstractService* service);
    void updateHelper(const QString& title, AbstractService* service,
                      std::function<void(QVector<QString> data)> update);
    void deleteHelper(QString table_name, AbstractService* service);
    void updateClient(QVector<QString> object);
    void updateAccount(QVector<QString> object);
    void updateTransaction(QVector<QString> object);
    void updateLoan(QVector<QString> object);
    std::pair<int, QString> insertClient();
    std::unique_ptr<ClientService> m_client_service = nullptr;
    std::unique_ptr<AccountService> m_account_service = nullptr;
    std::unique_ptr<TransactionService> m_transaction_service = nullptr;
    std::unique_ptr<LoanService> m_loan_service = nullptr;

    // AI loan recommender
    void startPythonServer();
    void stopPythonServer();
    QProcess *m_pythonServerProcess = nullptr;
    QTextEdit* m_chatHistory = nullptr;
};

#endif // MAINWINDOW_H
