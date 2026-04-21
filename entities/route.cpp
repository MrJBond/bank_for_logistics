#include "route.h"

Route::Route(const int id, const int driver_id,
             const QString& origin_address,
             const QString& destination_address,
             const double distance_km,
             const double estimated_hours,
             const QString& geoJsonString,
             const QString& status) : Entity(id){
    setDriverId(driver_id);
    setOriginAddress(origin_address);
    setDestinationAddress(destination_address);
    setDistance(distance_km);
    setEstimatedHours(estimated_hours);
    setGeoJsonString(geoJsonString);
    setStatus(status);
}

int Route::getDriverId() const{
    return m_driver_id;
}
void Route::setDriverId(const int driver_id){
    if(driver_id <= 0)
        throw std::invalid_argument("The driver_id is invalid!");
    m_driver_id = driver_id;
}
QString Route::getOriginAddress() const{
    return m_origin_address;
}
void Route::setOriginAddress(const QString& origin_address){
    m_origin_address = origin_address;
}
QString Route::getDestinationAddress() const{
    return m_destination_address;
}
void Route::setDestinationAddress(const QString& destination_address){
    m_destination_address = destination_address;
}
double Route::getDistance() const{
    return m_distance_km;
}
void Route::setDistance(const double distance_km){
    if(distance_km < 0)
         throw std::invalid_argument("The distance_km is invalid!");
    m_distance_km = distance_km;
}
double Route::getEstimatedHours() const{
    return m_estimated_hours;
}
void Route::setEstimatedHours(const double estimated_hours){
    if(estimated_hours < 0)
        throw std::invalid_argument("The estimated_hours is invalid!");
    m_estimated_hours = estimated_hours;
}
QString Route::getGeoJsonString() const{
    return m_geoJsonString;
}
void Route::setGeoJsonString(const QString& geoJsonString){
    m_geoJsonString = geoJsonString;
}
QString Route::getStatus() const{
    return m_status;
}
void Route::setStatus(const QString& status){
    m_status = status;
}
