#pragma once
#include <opencv2/opencv.hpp>
#include <QImage>

namespace Filters {

cv::Mat toGrayscale(const cv::Mat& src);
cv::Mat equalizeHistColor(const cv::Mat& src);
cv::Mat gaussianBlur(const cv::Mat& src, int ksize, double sigma);
cv::Mat canny(const cv::Mat& src, int low, int high);
cv::Mat brightnessContrast(const cv::Mat& src, int brightness, double contrast);
cv::Mat fftMagnitudeSpectrum(const cv::Mat& src);

// Qt <-> OpenCV
QImage matToQImage(const cv::Mat& mat);
cv::Mat qImageToMat(const QImage& img);

}
