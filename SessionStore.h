#pragma once
#include <QString>
#include <QJsonObject>
#include <QList>

struct FilterConfig {
    QString name;
    int    ksize = 5;
    double sigma = 1.0;
    int    lowThresh = 50;
    int    highThresh = 150;
    int    brightness = 0;
    double contrast = 1.0;
};

struct HistoryEntry {
    QString timestamp;
    QString operation;
};

class SessionStore {
public:
    explicit SessionStore(QString file = "session.json") : sessionFile(std::move(file)) {}

    bool save(const QString& lastImagePath, const FilterConfig& cfg) const;
    bool load(QString& lastImagePath, FilterConfig& cfg) const;

    bool saveRecent(const QStringList& recent) const;
    QStringList loadRecent() const;

    void appendHistory(const QString& imagePath, const HistoryEntry& entry) const;
    QList<HistoryEntry> loadHistory(const QString& imagePath) const;

private:
    QString sessionFile;

    static QJsonObject toJson(const FilterConfig& cfg);
    static void fromJson(const QJsonObject& o, FilterConfig& cfg);

    static QJsonObject toJson(const HistoryEntry& e);
    static HistoryEntry fromJsonHist(const QJsonObject& o);

    QJsonObject readRoot() const;
    bool writeRoot(const QJsonObject& root) const;
};
