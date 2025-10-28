#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QMap>

#include "ImageDocument.h"
#include "SessionStore.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openImage();
    void openRecentTriggered();
    void exportProcessed();
    void saveSession();
    void loadSession();
    void exitApp();
    void applyFilter();
    void updateControlsVisibility();
    void fitBothViews();
    void zoomIn();
    void zoomOut();
    void resetView();
    void showAbout();

private:
    void setupUiExtras();
    void buildMenusAndToolbar();
    void rebuildOpenRecentMenu();
    void refreshViews();
    void pushHistory(const QString& opText);
    void loadHistoryForCurrentImage();
    void setScale(QGraphicsView* view, double factor);
    void setNiceRenderHints(QGraphicsView* v);
    QString filterSummaryText() const;
    QString mapLegacyFilterName(const QString& legacy) const;

    Ui::MainWindow *ui;

    ImageDocument doc;
    SessionStore session { "session.json" };

    QGraphicsView* viewOriginal = nullptr;
    QGraphicsView* viewProcessed = nullptr;
    QGraphicsScene* sceneOriginal = nullptr;
    QGraphicsScene* sceneProcessed = nullptr;
    QComboBox* cbFilter = nullptr;
    QSpinBox* sbKsize = nullptr;
    QDoubleSpinBox* dsSigma = nullptr;
    QSpinBox* sbLow = nullptr;
    QSpinBox* sbHigh = nullptr;
    QSlider* sBrightness = nullptr;
    QDoubleSpinBox* dsContrast = nullptr;
    QLabel* lbBrightness = nullptr;
    QLabel* detailsLabel = nullptr;
    QDockWidget* historyDock = nullptr;
    QListWidget* historyList = nullptr;
    QToolBar* mainTb = nullptr;
    QMenu* openRecentMenu = nullptr;
    QStringList recentFiles;
    FilterConfig cfg;
    double currentScale = 1.0;
};

#endif // MAINWINDOW_H
