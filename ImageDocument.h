#pragma once
#include <opencv2/opencv.hpp>
#include <QString>

class ImageDocument {
public:
    bool load(const QString& path);
    bool saveProcessed(const QString& path) const;

    bool hasImage() const { return !original.empty(); }

    const cv::Mat& originalMat() const { return original; }
    const cv::Mat& processedMat() const { return processed; }

    void setProcessed(const cv::Mat& m) { processed = m.clone(); }
    QString lastPath() const { return imgPath; }

private:
    cv::Mat original;
    cv::Mat processed;
    QString imgPath;
};
