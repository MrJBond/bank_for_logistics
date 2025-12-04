#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H

#include "db/clientrepository.h"
#include "db/transactionrepository.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"
#include "AI/loanrecommender.h"
#include "AI/chatbot.h"

class ClientService : public AbstractService
{
    Q_OBJECT
private:
    std::shared_ptr<ClientRepository> m_client_repo = nullptr;
    std::shared_ptr<TransactionRepository> m_transaction_repo = nullptr;
    void putDataClientsWithTotalSumReport(QtRPT* report, QVector<QString> id_client,
                                          QVector<QString> name_client, QVector<QString> total_sum) const;
    // with nested report
    void putDataClientsAccountsReport(QtRPT* report, QMainWindow* window);
    void putDataSubReport(QtRPT* report, std::vector<Account> accs) const;
    std::vector<Transaction> listTransactions(const int id_client) const;

    LoanRecommender *m_loanRecommender = nullptr;
    ChatBot *m_chatBot = nullptr;

    void sendToPythonBot(const QString& userText);

    // --- SURVEY STATE VARIABLES --- [Bot's asses_risk feature]
    bool m_isSurveyActive = false;
    int m_currentQuestionIndex = 0;
    int m_riskScore = 0; // We will calculate points based on answers

    const std::vector<QString> m_surveyQuestions = {
        "Question 1/3: Do you have an emergency fund covering 3 months of expenses? (yes/no)",
        "Question 2/3: If your investment dropped 20% in one day, would you sell everything? (yes/no)",
        "Question 3/3: Are you planning to retire within the next 5 years? (yes/no)"
    };
    void startRiskSurvey();
    void processSurveyAnswer(const QString& answer);
    void finishSurvey();

    // return type: Key = "yyyy-MM", Value = {Income, Expense}
    typedef QMap<QString, std::pair<double, double>> MonthlyData;
    MonthlyData getMonthlyIncomeExpenses(const int clientId, const QDate& startDate) const;
    QLineSeries* fetchBalanceHistory(const int id_client, QStringList& categories) const;

public:
    ClientService();
    ~ClientService(){
        if(m_loanRecommender){
            delete m_loanRecommender;
            m_loanRecommender = nullptr;
        }
        if(m_chatBot){
            delete m_chatBot;
            m_chatBot = nullptr;
        }
    };
    void getAllClients(QTextBrowser* browser, QTableWidget *table = nullptr) const;
    void getClientsWithTotalSum(QTextBrowser* browser) const;
    void getClientsLegalIndividual(QTextBrowser* browser) const;
    void getLegalClientsWithTotalSum(QTextBrowser* browser) const;

    void getClientsWithTotalSumReport(QMainWindow* window) const;

    // with nested report
    void getClientsAccountsReport(QMainWindow* window);

    int insertClient(QString name, QString address, QString bossName,
                      QString bossPhone, QString accountantName,
                      QString accountantPhone);
    void updateClient(int id, QString name, QString address, QString bossName,
                      QString bossPhone, QString accountantName,
                      QString accountantPhone);
    void deleteClient(int id);
    void getDirectorView(QTextBrowser* browser) const;

    bool isClientPresent(const int id);

    /*****************************************************
                            CHARTS
     *****************************************************/
    void incomeExpensesChart(const int w, const int h) const;
    void balanceHistoryChart(const int w, const int h) const;
    /****************************************************
     *                      AI Lab2
    ****************************************************/
    void recommendLoanAmount(const int id) const; // $
    // Call this when the user types a message [for the survey: asses_risk]
    void handleUserMessage(const QString& msg);
signals:
    // to notify the MainWindow
    void chatReplyString(const QString& reply);
    void networkFailure(const QString& errorString);
    void loanRejection();
    void finalLoanAmount(double amount);
    void balanceCheckResult(const std::vector<Account>& accounts);
    void transactionListResult(const std::vector<Transaction>& transactions);
 private slots:
    void handleNetworkFailure(const QString& errorString);
    void handleLoanRejection();
    void handleFinalLoanAmount(double amount);
    void handleChatReply(const QString& intent, const QString& reply);
    void handleRecommendation(double score, double averageMonthlyIncome);
};
#endif // CLIENTSERVICE_H
