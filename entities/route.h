#ifndef ROUTE_H
#define ROUTE_H
#include "entity.h"
#include <QJsonObject>
#include <QJsonDocument>

class Route : public Entity
{
private:
    int m_driver_id = 0;
    QString m_origin_address = "";
    QString m_destination_address = "";
    double m_distance_km = 0.;
    double m_estimated_hours = 0.;
    QString m_geoJsonString = "";
    QString m_status = "";
public:
    Route(const int id, const int driver_id,
          const QString& origin_address,
          const QString& destination_address,
          const double distance_km,
          const double estimated_hours,
          const QString& geoJsonString,
          const QString& status);
    int getDriverId() const;
    void setDriverId(const int driver_id);
    QString getOriginAddress() const;
    void setOriginAddress(const QString& origin_address);
    QString getDestinationAddress() const;
    void setDestinationAddress(const QString& destination_address);
    double getDistance() const;
    void setDistance(const double distance_km);
    double getEstimatedHours() const;
    void setEstimatedHours(const double estimated_hours);
    QString getGeoJsonString() const;
    void setGeoJsonString(const QString& geoJsonString);
    QString getStatus() const;
    void setStatus(const QString& status);
};

#endif // ROUTE_H
