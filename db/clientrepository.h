#ifndef CLIENTREPOSITORY_H
#define CLIENTREPOSITORY_H

#include "entities/account.h"
#include "entities/client.h"
#include "abstractrepository.h"

class ClientRepository : public AbstractRepository
{
private:
    struct InnerClient{int id_client = 0; QString name = "";};
    // Clients and total transaction sum
    struct  ClientWithTotal : public InnerClient{double total_sum = 0.;};
    // bank clients, legal and individuals, sorted by amount of account balances
    struct ClientLegalIndividual  : public InnerClient{
        QString legal_address = "";
        QString type = "";};
    // for the director view
    struct directorView : public InnerClient{
        int id_ccount = 0; double amount = 0.;
        QString currency = "$"; };
public:
    ClientRepository();
    ~ClientRepository();
    std::vector<std::shared_ptr<Entity>> getAll() const override;
    void insert(std::shared_ptr<Entity> entity) override;
    void update(std::shared_ptr<Entity> entity) override;
    void remove(int id) override;
    std::vector<ClientWithTotal> getClientsWithTotalSum() const;
    std::vector<ClientLegalIndividual> getClientsLegalOrIndividual() const;
    std::vector<ClientWithTotal> getLegalClientsWithTotalSum(const int curYear) const;

    // Nested report => clients + their accounts + total money
    std::vector<Account> getAccountsForClient(const int id_client) const;
    std::vector<directorView> getDirectorView() const;

    /****************************************************
     *                      AI Lab2
     ****************************************************/
    double averageMonthlyIncome(const int id_client) const;
    double incomeVolatility(const int id_client) const;
    double existingDebtLoad(const int id_client) const;
};

#endif // CLIENTREPOSITORY_H
