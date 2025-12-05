#ifndef LOANSERVICE_H
#define LOANSERVICE_H

#include "db/loanrepository.h"
#include "qtextbrowser.h"
#include "service/abstractservice.h"


class LoanService : public AbstractService
{
private:
    std::shared_ptr<LoanRepository> m_loan_repo = nullptr;
    void putDataReportWithAllLoans(QtRPT* report) const;
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
};

#endif // LOANSERVICE_H
