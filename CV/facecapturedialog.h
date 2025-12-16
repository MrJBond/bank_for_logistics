#ifndef FACECAPTUREDIALOG_H
#define FACECAPTUREDIALOG_H
#include <QDialog>
#include <QCamera>
#include <QImageCapture>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMediaCaptureSession>

class FaceCaptureDialog : public QDialog {
    Q_OBJECT
public:
    FaceCaptureDialog(QWidget *parent = nullptr);
    QString getCapturedImageBase64() const;

private slots:
    void captureImage();
    void imageSaved(int id, const QImage &preview);

private:
    QCamera *m_camera;
    QImageCapture *m_imageCapture;
    QVideoWidget *m_viewfinder;
    QMediaCaptureSession *m_captureSession;
    QString m_base64Image;
};


#endif // FACECAPTUREDIALOG_H
