#include "clientrepository.h"

ClientRepository::ClientRepository(){}

ClientRepository::~ClientRepository(){}

std::vector<std::shared_ptr<Entity>> ClientRepository::getAll() const {
    std::vector<std::shared_ptr<Entity>> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM \"Client\" ORDER BY id_client")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        int id = query.value(0).toInt();
        std::string name = query.value(1).toString().toStdString();
        std::string address = query.value(2).toString().toStdString();
        std::string bossName = query.value(3).toString().toStdString();
        std::string bossPhone = query.value(4).toString().toStdString();
        std::string accountantName = query.value(5).toString().toStdString();
        std::string accountantPhone = query.value(6).toString().toStdString();
        try{
            auto client = std::make_shared<Client>(id, address, name, bossName,
                                                   bossPhone, accountantName, accountantPhone);
            res.push_back(client);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}

std::shared_ptr<Entity> ClientRepository::getById(const int id) const{
    QSqlQuery query;
    query.prepare("SELECT * FROM \"Client\" WHERE id_client = :id");
    query.bindValue(":id", id);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    if(query.next()){
        int id = query.value(0).toInt();
        std::string name = query.value(1).toString().toStdString();
        std::string address = query.value(2).toString().toStdString();
        std::string bossName = query.value(3).toString().toStdString();
        std::string bossPhone = query.value(4).toString().toStdString();
        std::string accountantName = query.value(5).toString().toStdString();
        std::string accountantPhone = query.value(6).toString().toStdString();
        try{
            auto client = std::make_shared<Client>(id, address, name, bossName,
                                                   bossPhone, accountantName, accountantPhone);
            return client;
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return nullptr; // not found
}

std::vector<ClientRepository::ClientWithTotal> ClientRepository::getClientsWithTotalSum() const{
    // Отримати список юридичних осіб, упорядкований за сумарними
    // обсягами платежів, вироблених з початку поточного року.

    std::vector<ClientWithTotal> res;
    QSqlQuery query;
    QString toExec = QString("WITH legal AS (\r\n")
                     + QString("    SELECT *\r\n")
                     + QString("    FROM \"Client\"\r\n")
                     + QString("    WHERE \"legal_address\" IS NOT NULL),\r\n")
                     + QString("Transaction2024 AS (\r\n")
                     + QString("    SELECT * FROM \"Transaction\"\r\n")
                     + QString("    WHERE \"date_transaction\" > '2024-01-01')\r\n")
                     + QString("SELECT\r\n")
                     + QString("    legal.\"id_client\", legal.\"name_client\",\r\n")
                     + QString("    SUM(Transaction2024.\"amount\") AS total_sum\r\n")
                     + QString("FROM legal\r\n")
                     + QString("JOIN\r\n")
                     + QString("    \"Account\" ON legal.\"id_client\" = \"Account\".\"id_client\"\r\n")
                     + QString("JOIN\r\n")
                     + QString("    Transaction2024 ON \"Account\".\"id_account\" = Transaction2024.\"id_account\"\r\n")
                     + QString("GROUP BY\r\n")
                     + QString("    legal.\"id_client\", legal.\"name_client\"\r\n")
                     + QString("ORDER BY total_sum;");
    if(!query.exec(toExec)){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        ClientWithTotal r;
        r.id_client = query.value(0).toInt();
        r.name = query.value(1).toString();
        r.total_sum = query.value(2).toDouble();
        res.push_back(r);
    }
    return res;
}

std::vector<Account> ClientRepository::getAccountsForClient(const int id_client){
    if(id_client <= 0){
        throw std::invalid_argument("The client's id is invalid!");
    }
    std::vector<Account> res;
    QSqlQuery query;
    query.prepare("SELECT * FROM \"Account\" WHERE id_client = :id");
    query.bindValue(":id", id_client);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    while(query.next()){
        int id = query.value(0).toInt();
        double amount = query.value(1).toDouble();
        QString currency = query.value(2).toString();
        int id_client = query.value(3).toInt();
        try{
            Account account = Account(id, id_client, amount, currency);
            res.push_back(account);
        }catch(const std::invalid_argument& e){
            qDebug() << e.what();
        }
    }
    return res;
}

void ClientRepository::insert(std::shared_ptr<Entity> entity) {
    if(entity == nullptr){
        return;
    }
    if(Client* client = dynamic_cast<Client*>(entity.get()); client != nullptr){
        QSqlQuery query;
        query.prepare(QString("INSERT INTO public.\"Client\" ") +
        "(name_client, legal_address, boss_name, boss_phone, accountant_name, accountant_phone)" +
        "VALUES(:name, :address, :boss_name, :boss_phone, :accountant_name, :accountant_phone);");
        query.bindValue(":name", client->getName().c_str());
        query.bindValue(":address", client->getAddress().c_str());
        query.bindValue(":boss_name", client->getBossName().c_str());
        query.bindValue(":boss_phone", client->getBossPhone().c_str());
        query.bindValue(":accountant_name", client->getAccountantName().c_str());
        query.bindValue(":accountant_phone", client->getAccountantPhone().c_str());
        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
        // Get the last inserted ID
        QVariant insertedId = query.lastInsertId();
        if (insertedId.isValid()) {
            int id = insertedId.toInt();
            entity->setId(id);
        }
    }
}
void ClientRepository::update(std::shared_ptr<Entity> entity){
    if(entity == nullptr){
        return;
    }
    if(Client* client = dynamic_cast<Client*>(entity.get()); client != nullptr){
        QSqlQuery query;
        query.prepare("UPDATE public.\"Client\" "
                      "SET name_client = :name, "
                      "legal_address = :address, "
                      "boss_name = :boss_name, "
                      "boss_phone = :boss_phone, "
                      "accountant_name = :accountant_name, "
                      "accountant_phone = :accountant_phone "
                      "WHERE id_client = :id"
                      );
        query.bindValue(":name", client->getName().c_str());
        query.bindValue(":address", client->getAddress().c_str());
        query.bindValue(":boss_name", client->getBossName().c_str());
        query.bindValue(":boss_phone", client->getBossPhone().c_str());
        query.bindValue(":accountant_name", client->getAccountantName().c_str());
        query.bindValue(":accountant_phone", client->getAccountantPhone().c_str());
        query.bindValue(":id", client->getId());
        if(!query.exec()){
            throw std::runtime_error(query.lastError().text().toStdString());
        }
    }
}
void ClientRepository::remove(int id){
    AbstractRepository::remove("id_client", "Client", id);
}
/*
Gets lists of bank clients, legal and individuals, sorted by
amount of account balances. Each of the lists must start with the appropriate
heading - "legal" or "individual". Prints the name for
legal entities or surname for individuals.
*/
std::vector<ClientRepository::ClientLegalIndividual> ClientRepository::getClientsLegalOrIndividual() const{
    std::vector<ClientLegalIndividual> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM LegalOrIndividual()")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        ClientLegalIndividual c;
        c.id_client = query.value(0).toInt();
        c.legal_address = query.value(1).toString();
        c.name = query.value(2).toString();
        c.type = query.value(3).toString();
        res.push_back(c);
    }
    return res;
}
/*
Gets a list of legal entities sorted by the total amount of payments,
    produced since the beginning of the current year.
    */
std::vector<ClientRepository::ClientWithTotal> ClientRepository::getLegalClientsWithTotalSum(const int curYear) const{
    std::vector<ClientWithTotal> res;
    QSqlQuery query;
    query.prepare("SELECT * FROM legalClientTranSum(:curYear)");
    query.bindValue(":curYear", curYear);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        ClientWithTotal c;
        c.id_client = query.value(0).toInt();
        c.total_sum = query.value(1).toDouble();
        res.push_back(c);
    }
    return res;
}
std::vector<ClientRepository::directorView> ClientRepository::getDirectorView() const{
    std::vector<directorView> res;
    QSqlQuery query;
    if(!query.exec("SELECT * FROM director_view")){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    while(query.next()){
        directorView d;
        d.id_client = query.value(0).toInt();
        d.id_ccount = query.value(1).toInt();
        d.name = query.value(2).toString();
        d.amount = query.value(3).toDouble();
        d.currency = query.value(4).toString();
        res.push_back(d);
    }
    return res;
}

/****************************************************
     *                      AI Lab2
****************************************************/

double ClientRepository::averageMonthlyIncome(const int id_client) const{
    if(id_client <= 0){
        throw std::invalid_argument("The client's id is invalid!");
    }
    struct accounts {int id_account; QString currency; double average_transaction_amount;};
    const QString queryStr = QString("WITH accounts AS ( ") +
                             QString("SELECT id_account, currency FROM \"Account\" WHERE id_client = :id) ") +
                             QString("SELECT a.id_account, a.currency, ") +
                             QString("AVG(t.amount) AS average_transaction_amount ") +
                             QString("FROM \"Transaction\" t ") +
                             QString("JOIN accounts a ON t.\"id_accountTo\" = a.id_account ") +
                             QString("WHERE t.date_transaction >= CURRENT_DATE - INTERVAL '12 month' ") +
                             QString("GROUP BY a.id_account, a.currency ") +
                             QString("ORDER BY a.id_account;");
    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":id", id_client);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    std::vector<accounts> res;
    while(query.next()){
        accounts a;
        a.id_account = query.value(0).toInt();
        a.currency = query.value(1).toString();
        a.average_transaction_amount = query.value(2).toDouble();
        res.push_back(a);
    }
    // sum up taking the currency into consideration
    double sum = 0.;
    for(const auto& e : res){
        sum += Entity::toDollar(e.average_transaction_amount, e.currency);
    }
    return sum;
}
double ClientRepository::incomeVolatility(const int id_client) const{
    if(id_client <= 0){
        throw std::invalid_argument("The client's id is invalid!");
    }
    struct accounts {int id_account; QString currency; QString flow_month; double net_flow; double account_stddev_of_monthly_flow;};
    const QString queryStr = QString("WITH accounts AS (") +
                             // Step 1: Select currency along with the ID
                             QString("SELECT id_account, currency FROM \"Account\" WHERE id_client = :id), ") +
                             QString("all_flows AS (") +
                             QString("SELECT ") + // Section 1: Get all INCOME
                             QString("t.\"id_accountTo\" AS id_account, ") +
                             QString("t.date_transaction, ") +
                             QString("t.amount FROM \"Transaction\" t ") + // Added space
                             QString("WHERE t.\"id_accountTo\" IN (SELECT id_account FROM accounts) ") + // Added space
                             QString("AND t.\"id_account\" NOT IN (SELECT id_account FROM accounts) ") + // Added space
                             QString("UNION ALL ") + // Section 2: Get all EXPENSES (Added space)
                             QString("SELECT t.\"id_account\" AS id_account, t.date_transaction, -t.amount ") + // Added space
                             QString("FROM \"Transaction\" t ") + // Added space
                             QString("WHERE t.\"id_account\" IN (SELECT id_account FROM accounts) ") + // Added space
                             QString("AND t.\"id_accountTo\" NOT IN (SELECT id_account FROM accounts)), ") +
                             QString("monthly_net_flow AS (") +
                             QString("SELECT id_account, DATE_TRUNC('month', date_transaction)::DATE AS flow_month, ") +
                             QString("SUM(amount) AS net_flow FROM all_flows ") + // Added space
                             QString("GROUP BY id_account, flow_month) ") +
                             // Final Step: Join back to 'accounts' to get the currency
                             QString("SELECT m.id_account, a.currency, m.flow_month, m.net_flow, ") +
                             QString("STDDEV(m.net_flow) OVER (PARTITION BY m.id_account) AS account_stddev_of_monthly_flow FROM ") +
                             QString("monthly_net_flow m ") +
                             QString("JOIN accounts a ON m.id_account = a.id_account ") +
                             QString("ORDER BY m.id_account, a.currency, m.flow_month;");
    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":id", id_client);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    std::map<int, accounts> res;
    while(query.next()){
        accounts a;
        a.id_account = query.value(0).toInt();
        a.currency = query.value(1).toString();
        a.flow_month = query.value(2).toString();
        a.net_flow = query.value(3).toDouble();
        a.account_stddev_of_monthly_flow = query.value(4).toDouble(); // this number is unique for every account
        res[a.id_account] = a; // interested only in the last column (account_stddev_of_monthly_flow)
    }
    // get an average of all accounts' stddevs
    double average = 0;
    for(const auto& a : res)
        average += Entity::toDollar(a.second.account_stddev_of_monthly_flow, a.second.currency);
    return average/res.size();
}
double ClientRepository::existingDebtLoad(const int id_client) const{
    if(id_client <= 0){
        throw std::invalid_argument("The client's id is invalid!");
    }
    const QString queryStr = QString("WITH accounts AS (") +
                             QString("SELECT id_account, currency ") +
                             QString("FROM \"Account\" ") +
                             QString("WHERE id_client = :id) ") +
                             QString("SELECT SUM(amount), currency FROM \"Loan\" l ") +
                             QString("JOIN accounts a ON l.id_account = a.id_account ") +
                             QString("GROUP BY currency;");
    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":id", id_client);
    if(!query.exec()){
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    double totalLoan = 0.;
    while(query.next()){
        const double sum = query.value(0).toDouble();
        const QString currency = query.value(1).toString();
        totalLoan += Entity::toDollar(sum, currency);
    }
    return totalLoan;
}
bool ClientRepository::isAccountMine(const int id_client, const int id_account){
    const std::vector<Account> accs = getAccountsForClient(id_client);
    for(const Account& a : accs)
        if(a.getId() == id_account)
            return id_client == a.getClientId();
    return false;
}
double ClientRepository::getTotalCurrentBalance(const int id_client) const{
    const std::vector<Account> accs = getAccountsForClient(id_client);
    double total = 0.;
    for(const Account& a : accs){
        const double currentBalance = Entity::toDollar(a.getAmount(), a.getCurrency());
        total += currentBalance;
    }
    return total;
}
