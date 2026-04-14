#include "facedetector.h"
#include <ranges>
#include <algorithm>
#include <format>
#include <QString>
#include <QCoreApplication>
#include <QDir>

FaceDetector::FaceDetector() {
    // 1. Get the folder where the .exe is currently running
    QString exeFolder = QCoreApplication::applicationDirPath();

    // 2. Build the absolute path to the XML file
    QString xmlPath = "../../CV/haarcascade_frontalface_default.xml";
    // 3. Try to load it using the absolute path
    if (!m_cascade.load(xmlPath.toStdString())) {
        qCritical() << "OpenCV Error: Could not find XML file at" << xmlPath;
        throw std::runtime_error("Could not load face cascade!");
    }

    qDebug() << "Successfully loaded OpenCV Face AI from:" << xmlPath;
}

template <OpenCVImage ImgType>
std::optional<cv::Rect> FaceDetector::findLargestFace(const ImgType& frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    m_cascade.detectMultiScale(gray, faces, 1.1, 4, 0, cv::Size(100, 100));

    if (faces.empty()) {
        return std::nullopt;
    }
    auto largest_face = std::ranges::max_element(faces,
                                                 [](const cv::Rect& a, const cv::Rect& b) {
                                                     return a.area() < b.area();
                                                 }
                                                 );
    return *largest_face;
}

void FaceDetector::drawUI(cv::Mat& frame, const cv::Rect& faceBox) {
    // Draw the green bounding box
    cv::rectangle(frame, faceBox, cv::Scalar(0, 255, 0), 2);

    // C++20 FORMAT: Fast, safe string formatting
    std::string label = std::format("Face Detected: {}x{}", faceBox.width, faceBox.height);
    // Put text above the box
    cv::putText(frame, label, cv::Point(faceBox.x, faceBox.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
}

// Explicit instantiation for the compiler
template std::optional<cv::Rect> FaceDetector::findLargestFace<cv::Mat>(const cv::Mat&);
