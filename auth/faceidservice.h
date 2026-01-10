#ifndef FACEIDSERVICE_H
#define FACEIDSERVICE_H
#include "AI/networkmanager.h"
#include <QJsonArray>

class FaceIdService : public NetworkManager {
    Q_OBJECT
public:
    explicit FaceIdService(QObject *parent = nullptr){};

    // Call this to start the process
    void requestFaceVector(const QString& base64Image, bool verifyUser = false);
    std::vector<double> parseUserFaceVector(const QString& currentFaceVectorJson) const;
    // compare face vectors
    double calculateDistance(const std::vector<double>& v1, const std::vector<double>& v2) const;
signals:
    // Emitted when Python returns the vector (e.g. "[0.123, -0.42, ...]")
    void vectorCalculated(const QString& vectorJson);
    void vectorCalculatedForVerification(const QString& vectorJson);
};

#endif // FACEIDSERVICE_H
