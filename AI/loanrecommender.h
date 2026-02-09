#ifndef LOANRECOMMENDER_H
#define LOANRECOMMENDER_H

// This class will manage the network communication
#include "AI/networkmanager.h"
#include "db/clientrepository.h"

class LoanRecommender : public NetworkManager
{
    Q_OBJECT
private:
    void requestLoanRecommendation(double avg_income, double stability, double existing_debt);
public:
    LoanRecommender();
    void recommendLoanAmount(const int id, ClientRepository* repo);
    // The function called by TransactionService
    void requestSpendingForecast(const std::vector<double>& monthlyHistory);
private slots:
    void handleRecommendation(double score, double averageMonthlyIncome);
signals:
    void recommendationReady(double score, double averageMonthlyIncome);
    /**
     * @brief Emitted when a final loan amount has been successfully calculated.
     * @param amount The suggested maximum loan amount, rounded.
     */
    void finalLoanAmountReady(double amount);

    /**
     * @brief Emitted when the score is too low and the loan is rejected.
     */
    void loanRejected();

    void forecastReceived(double prediction, double trend);
};


#endif // LOANRECOMMENDER_H
