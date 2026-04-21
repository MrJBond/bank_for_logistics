#include "routeplanner.h"

RoutePlanner::RoutePlanner() {}

void RoutePlanner::requestRoute(const QString& startAddress, const QString& endAddress, const int driver_id){
    // Create the JSON payload
    QJsonObject json;
    json["origin_address"] = startAddress;        // e.g., "Chicago, IL"
    json["destination_address"] = endAddress;     // e.g., "Dallas, TX"

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // Set up the network request
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/plan-route"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_manager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        qDebug() << "--- Network reply finished ---";

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response_data = reply->readAll();
            QJsonDocument json_response = QJsonDocument::fromJson(response_data);
            QJsonObject obj = json_response.object();
            if(obj.contains("error"))
                emit networkError(obj["error"].toString());
            else if (obj.contains("success") && obj["success"].toBool() == true) {
                qDebug() << "success";
                const int route_id = obj["route_id"].toInt();
                const double distance_km = obj["distance_km"].toDouble();
                const double estimated_hours = obj["estimated_hours"].toDouble();
                const QString geoJsonString = obj["route_geometry"].toString();
                try{
                    const Route r = Route(route_id, driver_id, startAddress,
                                          endAddress, distance_km, estimated_hours, geoJsonString, "PLANNED");
                    qDebug() << "SIGNAL: Route is ready";
                    emit routeReady(r);
                }catch(const std::exception& e){
                    qDebug() << e.what();
                }
            } else {
                qDebug() << "ERROR: 'success' is False";
            }
        } else {
            emit networkError(reply->errorString());
            qDebug() << "Route Error:" << reply->errorString();
            qDebug() << "Body:" << reply->readAll();
        }
        reply->deleteLater();
    });
}
