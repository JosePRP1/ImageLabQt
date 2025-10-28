#include "ImageDocument.h"
#include <opencv2/imgcodecs.hpp>

bool ImageDocument::load(const QString& path) {
    imgPath = path;
    original = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    processed = cv::Mat();
    return !original.empty();
}

bool ImageDocument::saveProcessed(const QString& path) const {
    if (processed.empty()) return false;
    return cv::imwrite(path.toStdString(), processed);
}
