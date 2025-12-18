#include "faceidservice.h"

void FaceIdService::requestFaceVector(const QString& base64Image) {
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/get-face-vector"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["image"] = base64Image;
    QJsonDocument doc(json);

    QNetworkReply *reply = m_manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = response.object();

            if (obj["success"].toBool()) {
                // Get the vector array as a string representation
                QJsonDocument vecDoc(obj["vector"].toArray());
                QString vectorString = vecDoc.toJson(QJsonDocument::Compact);
                emit vectorCalculated(vectorString);
            } else {
                emit networkError(obj["message"].toString());
            }
        } else {
            emit networkError("Network Error: " + reply->errorString());
        }
        reply->deleteLater();
    });
}
