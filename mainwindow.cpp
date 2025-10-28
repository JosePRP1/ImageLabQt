#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Filters.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGraphicsPixmapItem>
#include <QAction>
#include <QDateTime>
#include <QMenu>
#include <QStyle>
#include <QFont>
#include <QGraphicsTextItem>

static QString nowIso() {
    return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUiExtras();
    buildMenusAndToolbar();

    QString lastPath;
    if (session.load(lastPath, cfg)) {
        cfg.name = mapLegacyFilterName(cfg.name);
        recentFiles = session.loadRecent();
        rebuildOpenRecentMenu();

        if (!lastPath.isEmpty() && doc.load(lastPath)) {
            cbFilter->setCurrentText(cfg.name);
            applyFilter();
            loadHistoryForCurrentImage();
        } else {
            refreshViews();
        }
    } else {
        refreshViews();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::mapLegacyFilterName(const QString& legacy) const {
    static QMap<QString, QString> m = {
        {"None", "Nenhum"},
        {"Grayscale", "Escala de Cinza"},
        {"EqualizeHist", "Equalização de Histograma"},
        {"GaussianBlur", "Desfoque Gaussiano"},
        {"Canny", "Canny"},
        {"BrightnessContrast", "Brilho/Contraste"},
        {"FFT", "Espectro (FFT)"}
    };
    return m.value(legacy, legacy);
}

void MainWindow::setupUiExtras()
{
    sceneOriginal  = new QGraphicsScene(this);
    sceneProcessed = new QGraphicsScene(this);

    viewOriginal  = new QGraphicsView(sceneOriginal,  this);
    viewProcessed = new QGraphicsView(sceneProcessed, this);

    setNiceRenderHints(viewOriginal);
    setNiceRenderHints(viewProcessed);

    viewOriginal->setMinimumSize(500, 400);
    viewProcessed->setMinimumSize(500, 400);

    auto* splitter = new QSplitter(this);
    splitter->addWidget(viewOriginal);
    splitter->addWidget(viewProcessed);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    cbFilter = new QComboBox(this);
    cbFilter->addItems({
        "Nenhum",
        "Escala de Cinza",
        "Equalização de Histograma",
        "Desfoque Gaussiano",
        "Canny",
        "Brilho/Contraste",
        "Espectro (FFT)"
    });
    connect(cbFilter, &QComboBox::currentTextChanged, this, [this](const QString& name){
        cfg.name = name;
        updateControlsVisibility();
        applyFilter();
        if (doc.hasImage()) pushHistory(QString("Filtro: %1").arg(name));
    });

    sbKsize = new QSpinBox(this); sbKsize->setRange(1, 99); sbKsize->setSingleStep(2); sbKsize->setValue(cfg.ksize);
    dsSigma = new QDoubleSpinBox(this); dsSigma->setRange(0.1, 20.0); dsSigma->setSingleStep(0.1); dsSigma->setValue(cfg.sigma);
    connect(sbKsize, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ cfg.ksize = v; applyFilter(); if (doc.hasImage()) pushHistory(QString("Desfoque Gaussiano: k=%1 sigma=%2").arg(v).arg(cfg.sigma)); });
    connect(dsSigma, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v){ cfg.sigma = v; applyFilter(); if (doc.hasImage()) pushHistory(QString("Desfoque Gaussiano: k=%1 sigma=%2").arg(cfg.ksize).arg(v)); });

    sbLow  = new QSpinBox(this); sbLow->setRange(0, 255); sbLow->setValue(cfg.lowThresh);
    sbHigh = new QSpinBox(this); sbHigh->setRange(0, 255); sbHigh->setValue(cfg.highThresh);
    connect(sbLow,  qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ cfg.lowThresh = v; applyFilter(); if (doc.hasImage()) pushHistory(QString("Canny: low=%1 high=%2").arg(v).arg(cfg.highThresh)); });
    connect(sbHigh, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ cfg.highThresh = v; applyFilter(); if (doc.hasImage()) pushHistory(QString("Canny: low=%1 high=%2").arg(cfg.lowThresh).arg(v)); });

    lbBrightness = new QLabel(QString("Brilho: %1").arg(cfg.brightness), this);
    sBrightness = new QSlider(Qt::Horizontal, this); sBrightness->setRange(-100, 100); sBrightness->setValue(cfg.brightness);
    dsContrast  = new QDoubleSpinBox(this); dsContrast->setRange(0.1, 3.0); dsContrast->setSingleStep(0.1); dsContrast->setValue(cfg.contrast);

    auto* bcBox = new QWidget(this);
    auto* bcLay = new QVBoxLayout;
    bcLay->setContentsMargins(0,0,0,0);
    bcLay->setSpacing(4);
    bcLay->addWidget(lbBrightness);
    bcLay->addWidget(sBrightness);
    auto* contrastRow = new QHBoxLayout;
    contrastRow->setContentsMargins(0,0,0,0);
    contrastRow->setSpacing(8);
    contrastRow->addWidget(new QLabel("Contraste:", this));
    contrastRow->addWidget(dsContrast, 1);
    bcLay->addLayout(contrastRow);
    bcBox->setLayout(bcLay);

    connect(sBrightness, &QSlider::valueChanged, this, [this](int v){ cfg.brightness = v; lbBrightness->setText(QString("Brilho: %1").arg(v)); applyFilter(); if (doc.hasImage()) pushHistory(QString("Brilho/Contraste: brilho=%1 contraste=%2").arg(v).arg(cfg.contrast)); });
    connect(dsContrast,  qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v){ cfg.contrast = v; applyFilter(); if (doc.hasImage()) pushHistory(QString("Brilho/Contraste: brilho=%1 contraste=%2").arg(cfg.brightness).arg(v)); });

    auto* form = new QFormLayout;
    form->addRow("Filtro:", cbFilter);
    form->addRow("Ksize (ímpar):", sbKsize);
    form->addRow("Sigma:", dsSigma);
    form->addRow("Canny Low:", sbLow);
    form->addRow("Canny High:", sbHigh);
    form->addRow(bcBox);

    detailsLabel = new QLabel(this);
    detailsLabel->setObjectName("detailsLabel");
    detailsLabel->setAlignment(Qt::AlignLeft);
    QFont f = detailsLabel->font();
    f.setPointSize(f.pointSize()+1);
    detailsLabel->setFont(f);
    detailsLabel->setStyleSheet("#detailsLabel { padding: 8px 10px; border: 1px solid #3a3a3a; border-radius: 6px; background: rgba(255,255,255,0.03); }");

    auto* controlsWidget = new QWidget(this);
    auto* controlsVBox = new QVBoxLayout;
    controlsVBox->addLayout(form);
    controlsVBox->addSpacing(6);
    controlsVBox->addWidget(detailsLabel);
    controlsWidget->setLayout(controlsVBox);

    auto* central = new QWidget(this);
    auto* v = new QVBoxLayout;
    v->addWidget(splitter);
    v->addWidget(controlsWidget);
    central->setLayout(v);

    if (ui->centralwidget) {
        auto* outer = new QVBoxLayout(ui->centralwidget);
        outer->setContentsMargins(0,0,0,0);
        outer->addWidget(central);
    } else {
        setCentralWidget(central);
    }

    historyDock = new QDockWidget("Histórico", this);
    historyList = new QListWidget(historyDock);
    historyDock->setWidget(historyList);
    addDockWidget(Qt::RightDockWidgetArea, historyDock);

    statusBar()->showMessage("Pronto");
    updateControlsVisibility();
}


void MainWindow::buildMenusAndToolbar()
{
    auto* menuArquivo = ui->menubar->addMenu("Arquivo");
    auto* actOpen   = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Abrir imagem...", this);
    auto* actExport = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Exportar processada...", this);
    auto* actSaveS  = new QAction("Salvar sessão", this);
    auto* actLoadS  = new QAction("Carregar sessão", this);
    auto* actExit   = new QAction("Sair", this);

    connect(actOpen,  &QAction::triggered, this, &MainWindow::openImage);
    connect(actExport,&QAction::triggered, this, &MainWindow::exportProcessed);
    connect(actSaveS, &QAction::triggered, this, &MainWindow::saveSession);
    connect(actLoadS, &QAction::triggered, this, &MainWindow::loadSession);
    connect(actExit,  &QAction::triggered, this, &MainWindow::exitApp);

    openRecentMenu = new QMenu("Abrir recentes", this);
    menuArquivo->addAction(actOpen);
    menuArquivo->addMenu(openRecentMenu);
    menuArquivo->addSeparator();
    menuArquivo->addAction(actExport);
    menuArquivo->addSeparator();
    menuArquivo->addAction(actSaveS);
    menuArquivo->addAction(actLoadS);
    menuArquivo->addSeparator();
    menuArquivo->addAction(actExit);

    auto* menuExibir = ui->menubar->addMenu("Exibir");
    auto* actFit   = new QAction("Ajustar à Janela", this);
    auto* actZoomIn  = new QAction("Zoom +", this);
    auto* actZoomOut = new QAction("Zoom -", this);
    auto* actReset   = new QAction("Resetar Visão", this);
    connect(actFit,    &QAction::triggered, this, &MainWindow::fitBothViews);
    connect(actZoomIn, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(actZoomOut,&QAction::triggered, this, &MainWindow::zoomOut);
    connect(actReset,  &QAction::triggered, this, &MainWindow::resetView);
    menuExibir->addAction(actFit);
    menuExibir->addAction(actZoomIn);
    menuExibir->addAction(actZoomOut);
    menuExibir->addSeparator();
    menuExibir->addAction(actReset);

    auto* menuSobre = ui->menubar->addMenu("Sobre");
    auto* actSobre = new QAction("Sobre o ImageLabQt", this);
    connect(actSobre, &QAction::triggered, this, &MainWindow::showAbout);
    menuSobre->addAction(actSobre);

    mainTb = addToolBar("Principal");
    mainTb->addAction(actOpen);
    mainTb->addAction(actExport);
    mainTb->addSeparator();
    mainTb->addAction(actFit);
    mainTb->addAction(actZoomIn);
    mainTb->addAction(actZoomOut);
    mainTb->addAction(actReset);

    rebuildOpenRecentMenu();
}

void MainWindow::rebuildOpenRecentMenu()
{
    openRecentMenu->clear();
    if (recentFiles.isEmpty()) {
        auto *empty = new QAction("(vazio)", this);
        empty->setEnabled(false);
        openRecentMenu->addAction(empty);
        return;
    }
    for (const auto& path : recentFiles) {
        auto *a = new QAction(path, this);
        connect(a, &QAction::triggered, this, &MainWindow::openRecentTriggered);
        openRecentMenu->addAction(a);
    }
}

void MainWindow::openImage()
{
    auto path = QFileDialog::getOpenFileName(this, "Abrir imagem", QString(), "Imagens (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;
    if (!doc.load(path)) {
        QMessageBox::warning(this, "Erro", "Falha ao abrir a imagem.");
        return;
    }
    recentFiles.removeAll(path);
    recentFiles.prepend(path);
    while (recentFiles.size() > 10) recentFiles.removeLast();
    session.saveRecent(recentFiles);
    rebuildOpenRecentMenu();

    historyList->clear();
    loadHistoryForCurrentImage();

    applyFilter();
    statusBar()->showMessage(QString("Imagem carregada: %1").arg(path));
}

void MainWindow::openRecentTriggered()
{
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;
    const QString path = act->text();
    if (!doc.load(path)) {
        QMessageBox::warning(this, "Erro", "Falha ao abrir a imagem recente.");
        return;
    }
    recentFiles.removeAll(path);
    recentFiles.prepend(path);
    session.saveRecent(recentFiles);
    rebuildOpenRecentMenu();

    historyList->clear();
    loadHistoryForCurrentImage();

    applyFilter();
    statusBar()->showMessage(QString("Imagem carregada: %1").arg(path));
}

void MainWindow::exportProcessed()
{
    if (!doc.hasImage()) { QMessageBox::information(this, "Info", "Abra uma imagem primeiro."); return; }
    auto out = QFileDialog::getSaveFileName(this, "Exportar processada", "processed.png", "Imagens (*.png *.jpg *.jpeg *.bmp)");
    if (out.isEmpty()) return;
    if (!doc.saveProcessed(out)) QMessageBox::warning(this, "Erro", "Falha ao salvar a imagem processada.");
    else {
        pushHistory(QString("Exportou: %1").arg(out));
        statusBar()->showMessage("Imagem exportada com sucesso.");
    }
}

void MainWindow::saveSession()
{
    const QString last = doc.lastPath();
    if (!session.save(last, cfg)) {
        QMessageBox::warning(this, "Erro", "Não foi possível salvar a sessão.");
    } else {
        statusBar()->showMessage("Sessão salva em session.json");
    }
}

void MainWindow::loadSession()
{
    QString last;
    FilterConfig loaded;
    if (!session.load(last, loaded)) {
        QMessageBox::information(this, "Info", "Nenhuma sessão encontrada.");
        return;
    }
    loaded.name = mapLegacyFilterName(loaded.name);
    cfg = loaded;
    cbFilter->setCurrentText(cfg.name);
    if (!last.isEmpty()) {
        if (doc.load(last)) {
            applyFilter();
            loadHistoryForCurrentImage();
        }
    }
    recentFiles = session.loadRecent();
    rebuildOpenRecentMenu();
    statusBar()->showMessage("Sessão carregada.");
}

void MainWindow::exitApp() { close(); }

void MainWindow::applyFilter()
{
    const bool has = doc.hasImage();

    const cv::Mat& src = has ? doc.originalMat() : cv::Mat();
    cv::Mat out = src.empty() ? cv::Mat() : src.clone();

    if (has) {
        if (cfg.name == "Nenhum") {
        } else if (cfg.name == "Escala de Cinza") {
            out = Filters::toGrayscale(src);
        } else if (cfg.name == "Equalização de Histograma") {
            out = Filters::equalizeHistColor(src);
        } else if (cfg.name == "Desfoque Gaussiano") {
            out = Filters::gaussianBlur(src, cfg.ksize, cfg.sigma);
        } else if (cfg.name == "Canny") {
            out = Filters::canny(src, cfg.lowThresh, cfg.highThresh);
        } else if (cfg.name == "Brilho/Contraste") {
            out = Filters::brightnessContrast(src, cfg.brightness, cfg.contrast);
        } else if (cfg.name == "Espectro (FFT)") {
            out = Filters::fftMagnitudeSpectrum(src);
        }
        doc.setProcessed(out);
    }

    detailsLabel->setText(filterSummaryText());
    refreshViews();
}

QString MainWindow::filterSummaryText() const
{
    if (!doc.hasImage()) {
        return "Nenhuma imagem aberta — use Arquivo → Abrir imagem... ou Abrir recentes.\n"
               "Filtro atual: " + (cbFilter ? cbFilter->currentText() : QString("Nenhum"));
    }

    QString s = QString("Filtro atual: <b>%1</b>").arg(cfg.name);
    if (cfg.name == "Desfoque Gaussiano") {
        s += QString("  •  ksize=<b>%1</b>   sigma=<b>%2</b>").arg(cfg.ksize).arg(cfg.sigma, 0, 'f', 2);
    } else if (cfg.name == "Canny") {
        s += QString("  •  low=<b>%1</b>   high=<b>%2</b>").arg(cfg.lowThresh).arg(cfg.highThresh);
    } else if (cfg.name == "Brilho/Contraste") {
        s += QString("  •  brilho=<b>%1</b>   contraste=<b>%2</b>").arg(cfg.brightness).arg(cfg.contrast, 0, 'f', 2);
    } else if (cfg.name == "Espectro (FFT)") {
        s += "  •  exibe o espectro de magnitude (DFT centralizada).";
    } else if (cfg.name == "Equalização de Histograma") {
        s += "  •  equalização no canal Y (YCrCb).";
    } else if (cfg.name == "Escala de Cinza") {
        s += "  •  conversão para tons de cinza.";
    }
    return s;
}

void MainWindow::refreshViews()
{
    sceneOriginal->clear();
    sceneProcessed->clear();

    if (doc.hasImage()) {
        auto qOrig = Filters::matToQImage(doc.originalMat());
        sceneOriginal->addPixmap(QPixmap::fromImage(qOrig));
    } else {
        auto *t = sceneOriginal->addText("Sem imagem");
        QFont f = t->font(); f.setPointSize(f.pointSize()+6); t->setFont(f);
        t->setDefaultTextColor(QColor(160,160,160));
    }

    if (!doc.processedMat().empty()) {
        auto qProc = Filters::matToQImage(doc.processedMat());
        sceneProcessed->addPixmap(QPixmap::fromImage(qProc));
    } else {
        auto *t = sceneProcessed->addText("Sem pré-visualização");
        QFont f = t->font(); f.setPointSize(f.pointSize()+6); t->setFont(f);
        t->setDefaultTextColor(QColor(160,160,160));
    }
}

void MainWindow::updateControlsVisibility()
{
    const QString n = cbFilter->currentText();
    const bool g  = (n == "Desfoque Gaussiano");
    const bool c  = (n == "Canny");
    const bool bc = (n == "Brilho/Contraste");

    if (sbKsize) sbKsize->setVisible(g);
    if (dsSigma) dsSigma->setVisible(g);
    if (sbLow) sbLow->setVisible(c);
    if (sbHigh) sbHigh->setVisible(c);
    if (lbBrightness) lbBrightness->setVisible(bc);
    if (sBrightness) sBrightness->setVisible(bc);
    if (dsContrast) dsContrast->setVisible(bc);
}


void MainWindow::pushHistory(const QString& opText)
{
    if (!doc.hasImage()) return;
    const QString entry = QString("[%1] %2").arg(nowIso(), opText);
    historyList->addItem(entry);
    session.appendHistory(doc.lastPath(), { nowIso(), opText });
}

void MainWindow::loadHistoryForCurrentImage()
{
    if (!doc.hasImage()) return;
    const auto hist = session.loadHistory(doc.lastPath());
    for (const auto& h : hist) {
        historyList->addItem(QString("[%1] %2").arg(h.timestamp, h.operation));
    }
}

void MainWindow::fitBothViews()
{
    if (sceneOriginal && !sceneOriginal->items().isEmpty())
        viewOriginal->fitInView(sceneOriginal->itemsBoundingRect(), Qt::KeepAspectRatio);
    if (sceneProcessed && !sceneProcessed->items().isEmpty())
        viewProcessed->fitInView(sceneProcessed->itemsBoundingRect(), Qt::KeepAspectRatio);
    currentScale = 1.0;
}

void MainWindow::zoomIn()  { setScale(viewOriginal, 1.25); setScale(viewProcessed, 1.25); }
void MainWindow::zoomOut() { setScale(viewOriginal, 0.80); setScale(viewProcessed, 0.80); }

void MainWindow::resetView()
{
    viewOriginal->resetTransform();
    viewProcessed->resetTransform();
    currentScale = 1.0;
}

void MainWindow::setScale(QGraphicsView* view, double factor)
{
    view->scale(factor, factor);
    currentScale *= factor;
}

void MainWindow::setNiceRenderHints(QGraphicsView* v)
{
    v->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
}

void MainWindow::showAbout()
{
    const QString text =
        "<h3>ImageLabQt</h3>"
        "<p>Aplicativo desenvolvido por <b>José Paulo Remonti Pereira</b> "
        "(<a href=\"mailto:192445@upf.br\">192445@upf.br</a>) "
        "para a disciplina de <b>Laboratório de Programação III</b> "
        "do curso de <b>Ciência da Computação</b>.</p>"
        "<p>O trabalho aborda conceitos de <b>Processamento Digital de Imagens</b> "
        "e <b>Transformada de Fourier (FFT)</b>, permitindo carregar imagens, "
        "aplicar filtros (escala de cinza, equalização de histograma, desfoque gaussiano, "
        "detecção de bordas Canny, ajuste de brilho/contraste e espectro FFT), "
        "visualizar o resultado lado a lado e exportar a imagem processada. "
        "O aplicativo também registra o <b>histórico</b> das operações por imagem, "
        "mantém arquivos <b>recentes</b> e salva a <b>sessão</b> com os parâmetros utilizados.</p>";

    QMessageBox::about(this, "Sobre o ImageLabQt", text);
}
