#include "facecapturedialog.h"
#include <QBuffer>
#include <QDebug>

FaceCaptureDialog::FaceCaptureDialog(QWidget *parent) : QDialog(parent) {
    try{
        m_detector = std::make_unique<FaceDetector>();
    }catch(const std::runtime_error& e){
        qDebug() << e.what();
        throw std::range_error(e.what());
    }
    setWindowTitle("Face ID Scan");
    resize(640, 480); // Made slightly larger to fit standard webcam resolution

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 1. Setup Viewfinder (Now a simple QLabel)
    m_viewfinder = new QLabel(this);
    m_viewfinder->setAlignment(Qt::AlignCenter);
    m_viewfinder->setMinimumSize(400, 300);
    layout->addWidget(m_viewfinder);

    // 2. Setup Capture Button
    m_btnCapture = new QPushButton("Looking for face...", this);
    m_btnCapture->setEnabled(false); // Disable until AI finds a face
    layout->addWidget(m_btnCapture);

    connect(m_btnCapture, &QPushButton::clicked, this, &FaceCaptureDialog::captureImage);

    // 3. Setup OpenCV Camera
    m_camera.open(0); // 0 is usually the default webcam
    if (!m_camera.isOpened()) {
        qWarning() << "ERROR: Could not open the webcam via OpenCV!";
        m_viewfinder->setText("Camera not found.");
        return;
    }

    // 4. Start the "Game Loop" (30 Frames Per Second)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FaceCaptureDialog::processFrame);
    m_timer->start(33); // 33ms is roughly 30 FPS
}

FaceCaptureDialog::~FaceCaptureDialog() {
    // Crucial: Stop the camera when the window closes so it isn't stuck on
    if (m_timer && m_timer->isActive()) m_timer->stop();
    if (m_camera.isOpened()) m_camera.release();
}

void FaceCaptureDialog::processFrame() {
    cv::Mat frame;
    m_camera >> frame; // Grab a frame from the webcam
    if (frame.empty()) return;

    // Save a "clean" copy of the frame.
    // We want to send this to Python, NOT the one with the green box drawn on it!
    m_currentCleanFrame = frame.clone();

    // 1. Detect Face
    auto face = m_detector->findLargestFace(frame);

    // 2. Draw UI & Manage Button State
    if (face.has_value()) {
        m_detector->drawUI(frame, face.value());
        m_btnCapture->setEnabled(true);
        m_btnCapture->setText("Face Locked - Click to Scan");
        m_btnCapture->setStyleSheet("background-color: #28a745; color: white;"); // Make it green
    } else {
        m_btnCapture->setEnabled(false);
        m_btnCapture->setText("Looking for face...");
        m_btnCapture->setStyleSheet("");
    }

    // 3. Convert and display in Qt UI
    QImage qimg = matToQImage(frame);
    m_viewfinder->setPixmap(QPixmap::fromImage(qimg).scaled(
        m_viewfinder->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//#define TEST_IMG
void FaceCaptureDialog::captureImage() {
    if (m_currentCleanFrame.empty()) return;

    // Use the CLEAN frame (no green boxes) to send to the server
    QImage preview = matToQImage(m_currentCleanFrame);

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

#ifdef TEST_IMG
    QImage testImage(":/img/img/person4.png"); // person1.jpg, person2.jpg, person3.jpg, person4.png
    testImage.save(&buffer, "JPG");
#else
    preview.save(&buffer, "JPG", 80); // Added compression quality (80) to save bandwidth
#endif

    m_base64Image = QString::fromLatin1(byteArray.toBase64().data());

    // Stop everything and close
    m_timer->stop();
    m_camera.release();
    accept();
}

QString FaceCaptureDialog::getCapturedImageBase64() const {
    return m_base64Image;
}

QImage FaceCaptureDialog::matToQImage(const cv::Mat& mat) {
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
        return img.copy();
    }
    return QImage();
}
