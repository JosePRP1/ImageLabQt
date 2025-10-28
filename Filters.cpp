#include "Filters.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

namespace Filters {

cv::Mat toGrayscale(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 1) return src.clone();
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(gray, gray, cv::COLOR_GRAY2BGR);
    return gray;
}

cv::Mat equalizeHistColor(const cv::Mat& src) {
    cv::Mat ycrcb;
    cv::cvtColor(src, ycrcb, cv::COLOR_BGR2YCrCb);
    std::vector<cv::Mat> ch;
    cv::split(ycrcb, ch);
    cv::equalizeHist(ch[0], ch[0]);
    cv::merge(ch, ycrcb);
    cv::Mat out;
    cv::cvtColor(ycrcb, out, cv::COLOR_YCrCb2BGR);
    return out;
}

cv::Mat gaussianBlur(const cv::Mat& src, int ksize, double sigma) {
    if (ksize % 2 == 0) ksize += 1;
    cv::Mat out;
    cv::GaussianBlur(src, out, cv::Size(ksize, ksize), sigma);
    return out;
}

cv::Mat canny(const cv::Mat& src, int low, int high) {
    cv::Mat gray, edges, out;
    if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else gray = src;
    cv::Canny(gray, edges, low, high);
    cv::cvtColor(edges, out, cv::COLOR_GRAY2BGR);
    return out;
}

cv::Mat brightnessContrast(const cv::Mat& src, int brightness, double contrast) {
    cv::Mat out;
    src.convertTo(out, -1, contrast, brightness);
    return out;
}

cv::Mat fftMagnitudeSpectrum(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else gray = src.clone();

    cv::Mat floatImg;
    gray.convertTo(floatImg, CV_32F);
    cv::Mat planes[] = { floatImg, cv::Mat::zeros(floatImg.size(), CV_32F) };
    cv::Mat complexI;
    cv::merge(planes, 2, complexI);

    cv::dft(complexI, complexI);
    cv::split(complexI, planes);
    cv::magnitude(planes[0], planes[1], planes[0]);
    cv::Mat mag = planes[0];

    mag += cv::Scalar::all(1);
    cv::log(mag, mag);

    mag = mag(cv::Rect(0, 0, mag.cols & -2, mag.rows & -2));
    int cx = mag.cols/2;
    int cy = mag.rows/2;

    cv::Mat q0(mag, cv::Rect(0, 0, cx, cy));
    cv::Mat q1(mag, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2(mag, cv::Rect(0, cy, cx, cy));
    cv::Mat q3(mag, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
    q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);

    cv::normalize(mag, mag, 0, 255, cv::NORM_MINMAX);
    mag.convertTo(mag, CV_8U);
    cv::Mat out;
    cv::cvtColor(mag, out, cv::COLOR_GRAY2BGR);
    return out;
}

QImage matToQImage(const cv::Mat& mat) {
    if (mat.type() == CV_8UC3) {
        QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888);
        return img.copy();
    } else if (mat.type() == CV_8UC1) {
        QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        return img.copy();
    } else if (mat.type() == CV_8UC4) {
        QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return img.copy();
    }
    cv::Mat tmp;
    mat.convertTo(tmp, CV_8UC3);
    QImage img(tmp.data, tmp.cols, tmp.rows, tmp.step, QImage::Format_BGR888);
    return img.copy();
}

cv::Mat qImageToMat(const QImage& img) {
    QImage conv = img.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat mat(conv.height(), conv.width(), CV_8UC4, const_cast<uchar*>(conv.bits()), conv.bytesPerLine());
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);
    return bgr.clone();
}

} // namespace Filters
