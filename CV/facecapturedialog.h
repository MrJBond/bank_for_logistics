#ifndef FACECAPTUREDIALOG_H
#define FACECAPTUREDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QImage>

#include "FaceDetector.h"

class FaceCaptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit FaceCaptureDialog(QWidget *parent = nullptr);
    ~FaceCaptureDialog(); // Need a destructor to release the camera!

    QString getCapturedImageBase64() const;

private slots:
    void processFrame(); // Our new 30 FPS loop
    void captureImage();

private:
    static QImage matToQImage(const cv::Mat& mat);

    // UI Elements
    QLabel *m_viewfinder;
    QPushButton *m_btnCapture;

    // OpenCV Elements
    cv::VideoCapture m_camera;
    cv::Mat m_currentCleanFrame; // To save the photo WITHOUT the green box
    std::unique_ptr<FaceDetector> m_detector;
    QTimer *m_timer;

    QString m_base64Image;
};

#endif // FACECAPTUREDIALOG_H
