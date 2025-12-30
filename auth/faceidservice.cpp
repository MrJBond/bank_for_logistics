#include "faceidservice.h"

void FaceIdService::requestFaceVector(const QString& base64Image) {
    qDebug() << "=== requestFaceVector called ===";
    qDebug() << "Image data length:" << base64Image.length();

    QUrl url("http://127.0.0.1:5000/get-face-vector");
    qDebug() << "Target URL:" << url.toString();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["image"] = base64Image;
    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    qDebug() << "POST data size:" << postData.size();
    qDebug() << "POST data preview:" << postData.left(200);

    QNetworkReply *reply = m_manager->post(request, postData);

    qDebug() << "Request sent, waiting for reply...";

    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        qDebug() << "=== Reply received ===";
        qDebug() << "Error code:" << reply->error();
        qDebug() << "Error string:" << reply->errorString();
        qDebug() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

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
