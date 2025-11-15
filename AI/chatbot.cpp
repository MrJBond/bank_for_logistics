#include "chatbot.h"

void ChatBot::requestChat(const QString& userText) {
    // 1. Prepare JSON data
    QJsonObject json;
    json["message"] = userText;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 2. Create Request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/chat"));
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
            if(obj.contains("intent")) {
                QString intent = obj["intent"].toString();
                QString reply = obj["reply"].toString();

                emit chatReplyReady(intent, reply);
                qDebug() << "Received response:" << intent << " : " << reply;
            }
            else if(obj.contains("error")){
                QString error = obj["error"].toString();
                qDebug() << error;
                emit networkError(error);
            }
            else if(obj.contains("reply")){ // without an intent (means error)
                QString reply = obj["reply"].toString();
                qDebug() << reply;
                emit networkError(reply);
            }
            else {
                qDebug() << "ERROR: key not found in JSON response.";
                qDebug() << "Available keys:" << obj.keys();
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "ChatBot: NETWORK ERROR:" << reply->errorString();
            // This will give the HTTP code, like 400, 404, or 500
            qDebug() << "HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            // This prints any error message the Python server sent back
            qDebug() << "ChatBot: Error response body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}
