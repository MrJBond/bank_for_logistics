#include "usersession.h"
#include "service/clientservice.h"

UserSession* UserSession::getInstance()
{
    // Thread-safe static instance
    static UserSession instance;
    return &instance;
}

UserSession::UserSession()
    : m_userId(0), m_isLoggedIn(false)
{
    m_faceIdService = std::make_shared<FaceIdService>();
    m_client_repo = std::make_shared<ClientRepository>();
    connect(m_faceIdService.get(), &FaceIdService::vectorCalculatedForVerification,
            this, &UserSession::handleUserVerification);
    // Default state is logged out
}

int UserSession::login(const QString& username, const QString& password)
{
    // 1. Hash the password the user typed
    // We use SHA-256 for this example
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

    // 2. Query the database
    QSqlQuery query;
    query.prepare("SELECT id_client, password_hash FROM \"ClientAuth\" WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec() || !query.next()) {
        QString message = "Login failed: User not found or DB error User name: " + username;
        throw std::runtime_error(message.toStdString());
    }

    // 3. Check the password hash
    QByteArray storedHash = query.value("password_hash").toByteArray();
    if (passwordHash == storedHash) {
        // 4. LOGIN SUCCESS!
        int loggedInClientId = query.value("id_client").toInt();
        qDebug() << "Login successful for client ID: " << loggedInClientId;
        return loggedInClientId;
    } else {
        QString message =  "Login failed: Invalid password User name: " + username;
        throw std::runtime_error(message.toStdString());
    }
}

int UserSession::saveUserPassword(ClientService* service, const QString& username, const QString& password,
                                  const QString& address,
                                  const QString& bossName, const QString& bossPhone,
                                  const QString& accountantName, const QString& accountantPhone)
{
    QSqlDatabase *db = DbConnector::getInstance()->getDb();
    if (!db->transaction()) {
        const QString message = "Failed to start database transaction: " + db->lastError().text();
        throw std::runtime_error(message.toStdString());
    }

    try
    {
        // --- A: insert into Client table
        const int id = service->insertClient(username, address, bossName, bossPhone, accountantName, accountantPhone);
        if (id <= 0)
            throw std::runtime_error("Client service failed to return a valid ID.");

        // --- B: "ClientAuth"
        QByteArray hexHashBytes = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
        QString passwordHashString = QString(hexHashBytes);

        QSqlQuery queryAuth;
        queryAuth.prepare(
            "INSERT INTO public.\"ClientAuth\" (id_client, username, password_hash) "
            "VALUES (:id, :username, :hash)"
            );
        queryAuth.bindValue(":id", id);
        queryAuth.bindValue(":username", username);
        queryAuth.bindValue(":hash", passwordHashString);

        if (!queryAuth.exec()) {
            // it will be caught and rollback for the point --- A will be called
            const QString message = "Failed to create auth record: " + queryAuth.lastError().text();
            throw std::runtime_error(message.toStdString());
        }
        // --- C: Commit
        if (!db->commit()) {
            const QString message = "Failed to commit transaction: " + db->lastError().text();
            throw std::runtime_error(message.toStdString());
        }
        qDebug() << "Transaction successful: Auth record created for user: " << username;
        return id;
    }
    catch (const std::exception& e)
    {
        // --- D: Rollback
        // if something in the point A, B or C has thrown
        qCritical() << "Transaction FAILED, rolling back: " << e.what();
        db->rollback();
        // notify the caller
        throw std::runtime_error("Transaction FAILED");
    }
}

void UserSession::createSession(int userId, const QString& username)
{
    m_userId = userId;
    m_username = username;
    m_isLoggedIn = true;
}

void UserSession::destroySession()
{
    m_userId = 0;
    m_username = "";
    m_isLoggedIn = false;
}

int UserSession::getUserId() const
{
    return m_userId;
}

QString UserSession::getUsername() const
{
    return m_username;
}

bool UserSession::isLoggedIn() const
{
    return m_isLoggedIn;
}

void UserSession::requestUserVerification() const
{
    if(m_isLoggedIn){
        try{
            FaceCaptureDialog dlg;
            if (dlg.exec() == QDialog::Accepted) {
                const QString base64Image = dlg.getCapturedImageBase64();
                qDebug() << "Image before requesting the face vector: " << base64Image.left(200);
                m_faceIdService->requestFaceVector(base64Image, true);
            }
        }catch(const std::exception& e){
            qDebug() << e.what();
        }
    }
}
void UserSession::handleUserVerification(const QString& vectorJson){
    if(m_isLoggedIn){
        const std::vector<double> currentVector = m_faceIdService->parseUserFaceVector(vectorJson);
        std::unordered_map<int, QString> faces;
        try {
            faces = m_client_repo->getFaces();
        } catch(const std::runtime_error& e) {
            qDebug() << e.what();
        }
        const QString dbJson = faces[m_userId];
        // Parse DB vector
        const std::vector<double> dbVector = m_faceIdService->parseUserFaceVector(dbJson);
        const double dist = m_faceIdService->calculateDistance(currentVector, dbVector);
        qDebug() << "Distance: " << dist;
        if (dist < USER_FACE_THRESHOLD)
            emit userVerifiedSuccessfully();
        else
            emit verificationFailed();
    }else{
        emit verificationFailed();
    }
}
