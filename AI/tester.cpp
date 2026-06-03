#include "tester.h"

Tester::Tester() {
    connect(this, &Tester::networkError, this, &Tester::handleNetworkError);
    connect(this, &Tester::transactionsReady, this, &Tester::onTransactionsReady);
}

void Tester::testTransactionCategorization(){
    qDebug() << "testTransactionCategorization";
    try{
        qDebug() << "--- Starting Large-Scale ML Categorization Test ---";

        // 40 test сases
        std::vector<TestCase> testCases = {
            // FUEL
            {"Shell Gas Station", "Fuel", "⛽"},
            {"Pilot Flying J Diesel", "Fuel", "⛽"},
            {"Love's Travel Stop", "Fuel", "⛽"},
            {"Chevron Unleaded", "Fuel", "⛽"},
            {"BP Fuel Station", "Fuel", "⛽"},
            {"ExxonMobil", "Fuel", "⛽"},
            {"TA Petro Stopping Ctr", "Fuel", "⛽"},
            {"Gasoline #449", "Fuel", "⛽"},

            // MAINTENANCE
            {"Kenworth Truck Repair", "Maintenance", "🔧"},
            {"Auto Parts Maint.", "Maintenance", "🔧"},
            {"Michelin Tires", "Maintenance", "🔧"},
            {"Oil Change Express", "Maintenance", "🔧"},
            {"Cummins Engine Service", "Maintenance", "🔧"},
            {"Brake Pad Replacement", "Maintenance", "🔧"},
            {"Peterbilt Service Center", "Maintenance", "🔧"},
            {"Windshield wipers rep", "Maintenance", "🔧"},

            // LODGING
            {"Motel 6 Overnight", "Lodging", "🏨"},
            {"Holiday Inn Express", "Lodging", "🏨"},
            {"Super 8 Motel", "Lodging", "🏨"},
            {"Sleep Inn", "Lodging", "🏨"},
            {"Red Roof Inn", "Lodging", "🏨"},
            {"Comfort Suites", "Lodging", "🏨"},
            {"Inn and Suites Parking", "Lodging", "🏨"},

            // FOOD
            {"McDonalds Burger", "Food", "🍔"},
            {"Starbuks Coffe", "Food", "🍔"}, // typos
            {"Wendy's Drive Thru", "Food", "🍔"},
            {"Subway Sandwich", "Food", "🍔"},
            {"Denny's Restaurant", "Food", "🍔"},
            {"Taco Bell", "Food", "🍔"},
            {"Dunkin Donuts", "Food", "🍔"},

            // TOLLS
            {"Golden Gate Tollway", "Tolls", "🛣️"},
            {"I-90 Toll Plaza", "Tolls", "🛣️"},
            {"NY State Thruway Toll", "Tolls", "🛣️"},
            {"EZ Pass Reload", "Tolls", "🛣️"},
            {"Bridge Toll Fee", "Tolls", "🛣️"},

            // Other
            {"asdfghjkl obscure string", "Unknown", "❔"},
            {"Wire Transfer to Acc 55", "Transfer", "💸"},
            {"Monthly Netflix Sub", "Bills", "💡"},
            {"Amazon.com Shopping", "Shopping", "🛒"},
            {"ATM Cash Withdrawal", "Cash", "🏧"}
        };

        m_expectedTransactionResult.clear();
        std::vector<Transaction> transToSend;

        // generate transactions from the test cases
        for (size_t i = 0; i < testCases.size(); ++i) {
            // expected result
            m_expectedTransactionResult.push_back({
                static_cast<int>(i + 1),
                testCases[i].expectedCategory,
                testCases[i].expectedIcon
            });

            // transactions to send to Python
            Transaction t;
            t.setId(static_cast<int>(i + 1));
            t.setDescription(testCases[i].description);
            transToSend.push_back(t);
        }

        qDebug() << "Generated" << transToSend.size() << "transactions for testing.";
        requestTransactionCategorization(transToSend);

    }catch(const std::exception& e){
        qDebug() << e.what();
    }
}
void Tester::requestTransactionCategorization(const std::vector<Transaction>& trans){
    qDebug() << "requestTransactionCategorization";
    if (trans.empty()) return;

    // 1. Prepare JSON Payload (Batch)
    QJsonArray descArray;
    for (const auto& t : trans) {
        // Handle empty descriptions if necessary
        descArray.append(t.getDescription().isEmpty() ? "Transfer" : t.getDescription());
    }
    QJsonObject json;
    json["descriptions"] = descArray;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/categorize-list"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 3. Send Request
    QNetworkReply *reply = m_manager->post(request, data);
    qDebug() << "Batch Categorization POST called for" << trans.size() << "transactions";

    // 4. Handle Response
    // We capture 'trans' by value [=] so we can match results back to the specific transactions later
    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        qDebug() << "--- Network reply finished ---";

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument json_response = QJsonDocument::fromJson(response_data);
            QJsonObject obj = json_response.object();

            if (obj.contains("results") && obj["results"].isArray()) {
                QJsonArray results = obj["results"].toArray();

                // Safety check
                if (results.size() != (int)trans.size()) {
                    qWarning() << "Warning: Server returned" << results.size() << "results for" << trans.size() << "inputs.";
                }

                // 5. Match Results back to Transactions
                // The server preserves order, so index 0 corresponds to trans[0]
                int limit = std::min((int)trans.size(), (int)results.size());

                std::vector<TestTransaction> result(limit);
                for (int i = 0; i < limit; ++i) {
                    QJsonObject res = results[i].toObject();
                    QString category = res["category"].toString();
                    QString icon = res["icon"].toString();
                    result[i] = {i, category, icon};
                }
                emit transactionsReady(result);

            } else {
                qDebug() << "ERROR: 'results' array not found in JSON response.";
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "Categorization Error:" << reply->errorString();
            qDebug() << "Body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}
void Tester::handleNetworkError(const QString& errorString){
    qDebug() << errorString;
}

void Tester::onTransactionsReady(const std::vector<TestTransaction>& trans) {
    qDebug() << "\n--- Evaluating ML Categorization Results ---";

    if (trans.size() != m_expectedTransactionResult.size()) {
        qDebug() << "TEST FAILED: Size mismatch. Expected:"
                 << m_expectedTransactionResult.size() << "Got:" << trans.size();
        return;
    }

    // math metrics
    std::map<QString, int> TP, FP, FN;
    std::set<QString> all_categories;

    int passedCount = 0;

    // 1. Detailed updating and collection of statistics
    for (size_t i = 0; i < trans.size(); ++i) {
        const auto& expected = m_expectedTransactionResult[i];
        const auto& actual = trans[i];

        QString expCat = expected.category;
        QString actCat = actual.category;

        all_categories.insert(expCat);
        all_categories.insert(actCat);

        if (expCat == actCat) {
           // qDebug() << "✅ Test" << expected.id << "PASSED. [" << actCat << actCat << "]";
            TP[actCat]++; // True positive solutions for this category
            passedCount++;
        } else {
            qDebug() << "❌ Test" << expected.id << "FAILED.";
            qDebug() << "   Expected:" << expCat;
            qDebug() << "   Got     :" << actCat;

            FP[actCat]++; // The model incorrectly assigned this category (False Positive)
            FN[expCat]++; // The model "lost" the true category (False Negative)
        }
    }

    // 2. Classification Report
    qDebug() << "\n=====================================================";
    qDebug() << "          ML METRICS (CLASSIFICATION REPORT)         ";
    qDebug() << "=====================================================";

    double macro_precision = 0.0;
    double macro_recall = 0.0;
    double macro_f1 = 0.0;
    int valid_cats = 0;

    for (const QString& cat : all_categories) {
        double precision = 0.0;
        double recall = 0.0;
        double f1 = 0.0;

        // Division by zero protection
        if (TP[cat] + FP[cat] > 0)
            precision = (double)TP[cat] / (TP[cat] + FP[cat]);

        if (TP[cat] + FN[cat] > 0)
            recall = (double)TP[cat] / (TP[cat] + FN[cat]);

        if (precision + recall > 0)
            f1 = 2.0 * (precision * recall) / (precision + recall);

        qDebug().nospace() << "Category [" << cat << "]: "
                           << "TP=" << TP[cat] << " FP=" << FP[cat] << " FN=" << FN[cat];
        qDebug().nospace() << "  -> Precision : " << QString::number(precision, 'f', 2);
        qDebug().nospace() << "  -> Recall    : " << QString::number(recall, 'f', 2);
        qDebug().nospace() << "  -> F1-Score  : " << QString::number(f1, 'f', 2) << "\n";

        macro_precision += precision;
        macro_recall += recall;
        macro_f1 += f1;
        valid_cats++;
    }

    // Summary (average) metrics
    if (valid_cats > 0) {
        qDebug() << "-----------------------------------------------------";
        qDebug().nospace() << "OVERALL MACRO-AVERAGE F1-SCORE: "
                           << QString::number(macro_f1 / valid_cats, 'f', 2);
    }

    qDebug() << "\n--- Final Summary ---";
    qDebug() << passedCount << "out of" << m_expectedTransactionResult.size() << "tests matched exactly.";
    if (passedCount == (int)m_expectedTransactionResult.size()) {
        qDebug() << "🎉 ALL CATEGORIZATION TESTS PASSED SUCCESSFULLY!";
    } else {
        qDebug() << "⚠️ SOME TESTS FAILED. CHECK THE LOGS.";
    }
}
