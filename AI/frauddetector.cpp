#include "frauddetector.h"

FraudDetector::FraudDetector() {}
FraudDetector::~FraudDetector(){}

void FraudDetector::requestTransactionCheck(const Transaction& t, const QJsonArray& history){
    const double amount = t.getAmount();
    // 1. Prepare JSON Payload
    QJsonObject json;
    json["amount"] = amount;
    json["history"] = history;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/check-fraud"));
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
            bool isSuspicious = true;
            if(obj.contains("error")){
                qDebug() << "ERROR: " << obj["error"];
            }
            if(obj.contains("status"))
                 isSuspicious = obj["status"].toString() == "SUSPICIOUS" ? true : false;
            if(obj.contains("score")) {
                double score = obj["score"].toDouble();
                emit transactionChecked(isSuspicious, score, t);
                qDebug() << "Received score:" << score;
                qDebug() << "Received status: " << obj["status"].toString() << isSuspicious;
            }
            else if(obj.contains("reason")){
                qDebug() << "No score: " << obj["reason"].toString();
                emit transactionChecked(isSuspicious, 0., t);
            }
            else {
                qDebug() << "ERROR: 'score' key not found in JSON response.";
                qDebug() << "Available keys:" << obj.keys();
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "FraudDetector: NETWORK ERROR:" << reply->errorString();
            // This will give the HTTP code, like 400, 404, or 500
            qDebug() << "HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            // This prints any error message the Python server sent back
            qDebug() << "FraudDetector: Error response body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

void FraudDetector::requestTransactionCategorization(const Transaction& t){
    // 1. Prepare JSON Payload
    QJsonObject json;
    json["description"] = t.getDescription();
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/categorize"));
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
            if(obj.contains("category") && obj.contains("icon")){
                const QString category = obj["category"].toString();
                const QString icon = obj["icon"].toString();
                if(category == "Error"){
                    const QString error = category + " " + icon;
                    emit networkError(error);
                    qDebug() << "TransactionCategorization: NETWORK ERROR:" << error;
                }
                else
                    emit transactionCategorized(category, icon, t);
            }
            else {
                qDebug() << "ERROR: 'category' or 'icon' key not found in JSON response.";
                qDebug() << "Available keys:" << obj.keys();
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "TransactionCategorization: NETWORK ERROR:" << reply->errorString();
            // This will give the HTTP code, like 400, 404, or 500
            qDebug() << "HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            // This prints any error message the Python server sent back
            qDebug() << "TransactionCategorization: Error response body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}
void FraudDetector::requestTransactionCategorization(const std::vector<Transaction>& trans) {
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

                for (int i = 0; i < limit; ++i) {
                    QJsonObject res = results[i].toObject();
                    QString category = res["category"].toString();
                    QString icon = res["icon"].toString();

                    // Emit for each transaction individually
                    emit transactionCategorized(category, icon, trans[i]);
                }

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
