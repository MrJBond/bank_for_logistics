#ifndef LOANSERVICE_H
#define LOANSERVICE_H

#include "db/loanrepository.h"
#include "db/clientrepository.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"
#include "AI/loanrecommender.h"

class LoanService : public AbstractService
{
    Q_OBJECT
private:
    std::shared_ptr<LoanRepository> m_loan_repo = nullptr;
    std::shared_ptr<ClientRepository> m_client_repo = nullptr;
    std::unique_ptr<LoanRecommender> m_loanRecommender = nullptr;
    void putDataReportWithAllLoans(QtRPT* report) const;
    double calculateInterestRate(const double aiScore) const;
    void takeLoan(const int accountId, const double amount, const int durationMonths, const double aiScore);

    // Define a simple container for the request details
    struct PendingLoanRequest {
        int accountId;
        double amount;
        int durationMonths;
        bool isValid = false; // To check if we actually have a request pending
    };
    // Add a member variable to store it
    PendingLoanRequest m_pendingRequest;
public:
    LoanService();
    ~LoanService() = default;
    void getAll(QTextBrowser* browser, QTableWidget *table = nullptr) const override;
    void loanReportWithAllLoans(QMainWindow* window) const;
    int insertLoan(int id_account, QDate issue_date,
                    QDate usage_date,double percent,
                    double amount);
    void updateLoan(int id, int id_account, QDate issue_date,
                    QDate usage_date,double percent,
                    double amount);
    void deleteObj(const int id) override;
    void getTotalEarnedMoney(QTextBrowser* browser) const;
    void getIndividualLoans(QTextBrowser* browser) const;
    void getAverageLegalLoans(QTextBrowser* browser) const;
    void getLoanClientView(QTextBrowser* browser) const;
    void getLoanView(QTextBrowser* browser) const;
    void requestLoan(const int accountId, const double amount, const int durationMonths);
private slots:
    void handleRecommendation(double score, double averageMonthlyIncome);
signals:
    void loanResult(bool approved, const QString& msg);
};

#endif // LOANSERVICE_H
