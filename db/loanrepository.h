#ifndef LOANREPOSITORY_H
#define LOANREPOSITORY_H

#include "abstractrepository.h"
#include "entities/account.h"
#include "entities/loan.h"

class LoanRepository : public AbstractRepository
{
private:
    // to get total earned money
    struct totalMoney{double percent = 0.; double amount = 0.;
        double income = 0.;};
    // to get individual loans
    struct individualLoans{Account acc = Account(); double totalLoan = 0.;
        double percent_of_total = 0.;};
    // to get average loans issued to legal entities
    struct legalLoans{int id_client = 0; double average = 0.;};
    // loan client view
    struct loanClientView{int id_loan = 0; QString legal_address = ""; double loan_amount = 0.;};
public:
    LoanRepository();
    ~LoanRepository();
    std::vector<std::shared_ptr<Entity>> getAll() const override;
    void insert(std::shared_ptr<Entity> entity) override;
    void update(std::shared_ptr<Entity> entity) override;
    void remove(int id) override;
    std::vector<totalMoney> getTotalEarnedMoney(int year);
    std::vector<individualLoans> getIndividualLoans() const;
    std::vector<legalLoans> getAverageLegalLoans() const;
    std::vector<loanClientView> getLoanClientView() const;
    std::vector<Loan> getLoanView() const;
};

#endif // LOANREPOSITORY_H
