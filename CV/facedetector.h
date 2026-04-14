#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <vector>
#include <concepts>
#include <optional>

template <typename T>
concept OpenCVImage = std::same_as<T, cv::Mat>;

class FaceDetector {
public:
    FaceDetector();

    // Use our Concept here. The compiler will reject anything that isn't a cv::Mat.
    template <OpenCVImage ImgType>
    std::optional<cv::Rect> findLargestFace(const ImgType& frame);
    void drawUI(cv::Mat& frame, const cv::Rect& faceBox);

private:
    cv::CascadeClassifier m_cascade;
};
#endif // FACEDETECTOR_H
