#include "loanrecommender.h"

LoanRecommender::LoanRecommender(){
    connect(this, &LoanRecommender::recommendationReady,
            this, &LoanRecommender::handleRecommendation);
}
void LoanRecommender::requestLoanRecommendation(double avg_income, double stability, double existing_debt)
{
    // 1. Prepare JSON data
    QJsonObject json;
    json["avg_income"] = avg_income;
    json["stability"] = stability;
    json["existing_debt"] = existing_debt;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/recommend-loan"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 3. Send Request (asynchronously)
    QNetworkReply *reply = m_manager->post(request, data);
    qDebug() << "Post called";
    // 4. Connect to the response
    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        qDebug() << "--- Network reply finished ---";
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            qDebug() << "Raw response data:" << response_data;
            QJsonDocument json_response = QJsonDocument::fromJson(response_data);
            QJsonObject obj = json_response.object();
            // Check if the key exists
            if(obj.contains("recommendation_score") && obj.contains("inputs_received")) {
                double score = obj["recommendation_score"].toDouble();
                QJsonObject inputsObj = obj["inputs_received"].toObject();
                double avg_income_from_response = inputsObj["avg_income"].toDouble();
                emit recommendationReady(score, avg_income_from_response);
                qDebug() << "Received recommendation score:" << score;
            } else {
                qDebug() << "ERROR: 'recommendation_score' key not found in JSON response.";
                qDebug() << "Available keys:" << obj.keys();
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "LoanRecommender: NETWORK ERROR:" << reply->errorString();
            // This will give the HTTP code, like 400, 404, or 500
            qDebug() << "HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            // This prints any error message the Python server sent back
            qDebug() << "LoanRecommender: Error response body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}
void LoanRecommender::handleRecommendation(double score, double averageMonthlyIncome)
{
    double maxLoanFactor = 3.0; // e.g., We never lend more than 3x monthly income

    // Map the 0-100 score to a multiplier
    /*
    Score = 100 (Perfect): The client gets 100% of the maximum allowed loan (3.0 * income).
    Score = 50 (Average): The client gets 50% of the maximum allowed loan (1.5 * income).
    Score = 32.23 (Weak): The client gets 32.23% of the maximum allowed loan (0.96 * income).
    */
    double suggestedMultiplier = (score / 100.0) * maxLoanFactor;

    // Using averageMonthlyIncome as the foundation ensures the final suggested amount
    // is directly tied to the client's real-world financial capacity
    double suggestedLoanAmount = averageMonthlyIncome * suggestedMultiplier;

    if (score < 20) { // Corresponds to the 'Reject' fuzzy set
        qDebug() << "LoanRecommender: Loan recommendation: REJECT";
        emit loanRejected();
    } else {
        qDebug() << "LoanRecommender: Fuzzy System Score:" << score;
        qDebug() << "LoanRecommender: Suggested Maximum Loan Amount:" << qRound(suggestedLoanAmount);
        emit finalLoanAmountReady(qRound(suggestedLoanAmount));
    }
}

void LoanRecommender::recommendLoanAmount(const int id, ClientRepository* repo){
    double avg_income = 0., stability_metric = 0., existing_debt = 0.;
    if(id <= 0){
        const QString message = "User's id is invalid! id = " + QString::number(id);
        throw std::runtime_error(message.toStdString());
    }
    try{
        avg_income = repo->averageMonthlyIncome(id);
        stability_metric = repo->incomeVolatility(id);
        existing_debt = repo->existingDebtLoad(id);
    }catch(const std::runtime_error& e){
        qDebug() << e.what();
    }
    catch(const std::invalid_argument& e){
        qDebug() << e.what();
    }
    qDebug() << "avg_income: " << avg_income;
    qDebug() << "stability_metric: " << stability_metric;
    qDebug() << "existing_debt: " << existing_debt;
    requestLoanRecommendation(avg_income, stability_metric, existing_debt);
}

void LoanRecommender::requestSpendingForecast(const std::vector<double>& monthlyHistory) {
    if (monthlyHistory.size() < 2) {
        emit networkError("Not enough data points (minimum 2 months required).");
        return;
    }
    // 1. Prepare JSON Payload
    QJsonObject json;
    QJsonArray arr;
    for (double amount : monthlyHistory) {
        arr.append(amount);
    }
    json["history"] = arr;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/forecast-spending"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 3. Send Async Request
    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
            QJsonObject obj = jsonResponse.object();

            if (obj["success"].toBool()) {
                double prediction = obj["prediction"].toDouble();
                double trend = obj["trend"].toDouble();
                emit forecastReceived(prediction, trend);
            } else {
                emit networkError(obj["error"].toString());
            }
        } else {
            emit networkError("Network Error: " + reply->errorString());
        }
        reply->deleteLater();
    });
}
