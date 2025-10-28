#include "SessionStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QJsonObject SessionStore::toJson(const FilterConfig& cfg) {
    QJsonObject o;
    o["name"] = cfg.name;
    o["ksize"] = cfg.ksize;
    o["sigma"] = cfg.sigma;
    o["lowThresh"] = cfg.lowThresh;
    o["highThresh"] = cfg.highThresh;
    o["brightness"] = cfg.brightness;
    o["contrast"] = cfg.contrast;
    return o;
}

void SessionStore::fromJson(const QJsonObject& o, FilterConfig& cfg) {
    cfg.name = o.value("name").toString("Nenhum");
    cfg.ksize = o.value("ksize").toInt(5);
    cfg.sigma = o.value("sigma").toDouble(1.0);
    cfg.lowThresh = o.value("lowThresh").toInt(50);
    cfg.highThresh = o.value("highThresh").toInt(150);
    cfg.brightness = o.value("brightness").toInt(0);
    cfg.contrast = o.value("contrast").toDouble(1.0);
}

QJsonObject SessionStore::toJson(const HistoryEntry& e) {
    QJsonObject o;
    o["timestamp"] = e.timestamp;
    o["operation"] = e.operation;
    return o;
}

HistoryEntry SessionStore::fromJsonHist(const QJsonObject& o) {
    HistoryEntry e;
    e.timestamp = o.value("timestamp").toString();
    e.operation = o.value("operation").toString();
    return e;
}

QJsonObject SessionStore::readRoot() const {
    QFile f(sessionFile);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return QJsonObject{};
    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return QJsonObject{};
    return doc.object();
}

bool SessionStore::writeRoot(const QJsonObject& root) const {
    QFile f(sessionFile);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool SessionStore::save(const QString& lastImagePath, const FilterConfig& cfg) const {
    auto root = readRoot();
    root["lastImagePath"] = lastImagePath;
    root["filter"] = toJson(cfg);
    return writeRoot(root);
}

bool SessionStore::load(QString& lastImagePath, FilterConfig& cfg) const {
    auto root = readRoot();
    if (root.isEmpty()) return false;
    lastImagePath = root.value("lastImagePath").toString();
    fromJson(root.value("filter").toObject(), cfg);
    return true;
}

bool SessionStore::saveRecent(const QStringList& recent) const {
    auto root = readRoot();
    QJsonArray arr;
    for (const auto& s : recent) arr.append(s);
    root["recent"] = arr;
    return writeRoot(root);
}

QStringList SessionStore::loadRecent() const {
    QStringList out;
    auto root = readRoot();
    auto arr = root.value("recent").toArray();
    for (const auto& v : arr) out << v.toString();
    return out;
}

void SessionStore::appendHistory(const QString& imagePath, const HistoryEntry& entry) const {
    auto root = readRoot();

    QJsonObject historyMap = root.value("history").toObject();
    QJsonArray list = historyMap.value(imagePath).toArray();
    list.append(toJson(entry));
    historyMap[imagePath] = list;
    root["history"] = historyMap;

    root["lastImagePath"] = imagePath;

    writeRoot(root);
}

QList<HistoryEntry> SessionStore::loadHistory(const QString& imagePath) const {
    QList<HistoryEntry> out;
    auto root = readRoot();
    auto historyMap = root.value("history").toObject();
    auto list = historyMap.value(imagePath).toArray();
    out.reserve(list.size());
    for (const auto& v : list) {
        out.push_back(fromJsonHist(v.toObject()));
    }
    return out;
}
