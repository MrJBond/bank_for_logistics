#include "facecapturedialog.h"
#include <QBuffer>
#include <QMediaDevices> // To find the default camera

FaceCaptureDialog::FaceCaptureDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Face ID Scan");
    resize(400, 350);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 1. Setup Camera and Session
    m_captureSession = new QMediaCaptureSession(this);
    m_camera = new QCamera(QMediaDevices::defaultVideoInput(), this); // Specify device
    m_captureSession->setCamera(m_camera);

    // 2. Setup Viewfinder
    m_viewfinder = new QVideoWidget(this);
    m_captureSession->setVideoOutput(m_viewfinder); // Session manages the output
    layout->addWidget(m_viewfinder);

    // 3. Setup Capture
    m_imageCapture = new QImageCapture(this);
    m_captureSession->setImageCapture(m_imageCapture); // Connect capture to session

    // 4. UI and Signals
    QPushButton *btnCapture = new QPushButton("Scan Face", this);
    layout->addWidget(btnCapture);

    connect(btnCapture, &QPushButton::clicked, this, &FaceCaptureDialog::captureImage);
    connect(m_imageCapture, &QImageCapture::imageCaptured, this, &FaceCaptureDialog::imageSaved);

    m_camera->start();
}

void FaceCaptureDialog::captureImage() {
    m_imageCapture->capture();
}

void FaceCaptureDialog::imageSaved(int id, const QImage &preview) {
    Q_UNUSED(id);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    preview.save(&buffer, "JPG");
    m_base64Image = QString::fromLatin1(byteArray.toBase64().data());

    m_camera->stop();
    accept();
}

QString FaceCaptureDialog::getCapturedImageBase64() const {
    return m_base64Image;
}
