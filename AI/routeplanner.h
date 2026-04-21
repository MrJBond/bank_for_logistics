#ifndef ROUTEPLANNER_H
#define ROUTEPLANNER_H

#include "AI/networkmanager.h"
#include "entities/route.h"

class RoutePlanner : public NetworkManager
{
    Q_OBJECT
public:
    RoutePlanner();
    void requestRoute(const QString& startAddress, const QString& endAddress, const int driver_id);
signals:
    void routeReady(const Route& route);
};

#endif // ROUTEPLANNER_H
