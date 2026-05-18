#include "PassiveRadar.h"

void enforceEqualAxesScale(QCustomPlot* customPlot) {
    if (!customPlot) return;

    // 1. 获取当前轴范围
    QCPRange xRange = customPlot->xAxis->range();
    QCPRange yRange = customPlot->yAxis->range();

    double xMin = xRange.lower;
    double xMax = xRange.upper;
    double yMin = yRange.lower;
    double yMax = yRange.upper;

    // 计算数据范围跨度
    double xDataRange = xMax - xMin;
    double yDataRange = yMax - yMin;

    // 2. 获取绘图区域像素尺寸
    int plotWidth = customPlot->axisRect()->width();
    int plotHeight = customPlot->axisRect()->height();

    if (xDataRange <= 0 || yDataRange <= 0 || plotWidth <= 0 || plotHeight <= 0) {
        // 数据范围或尺寸无效，无法计算比例
        return;
    }

    // 3. 计算单位数据单位所需像素数
    double xPixelsPerDataUnit = static_cast<double>(plotWidth) / xDataRange;
    double yPixelsPerDataUnit = static_cast<double>(plotHeight) / yDataRange;

    // 4. 确定限制因素并调整范围
    if (xPixelsPerDataUnit <= yPixelsPerDataUnit) {
        // 情况 1: X 轴是限制因素，调整 Y 轴
        double requiredYRange = plotHeight / xPixelsPerDataUnit;
        double centerY = (yMax + yMin) / 2.0;
        double newYMin = centerY - requiredYRange / 2.0;
        double newYMax = centerY + requiredYRange / 2.0;

        // 检查是否需要调整（避免不必要的 replot）
        if (std::abs(newYMin - yMin) > 1e-10 || std::abs(newYMax - yMax) > 1e-10) {
            customPlot->yAxis->setRange(newYMin, newYMax);
        }
    }
    else {
        // 情况 2: Y 轴是限制因素，调整 X 轴
        double requiredXRange = plotWidth / yPixelsPerDataUnit;
        double centerX = (xMax + xMin) / 2.0;
        double newXMin = centerX - requiredXRange / 2.0;
        double newXMax = centerX + requiredXRange / 2.0;

        // 检查是否需要调整（避免不必要的 replot）
        if (std::abs(newXMin - xMin) > 1e-10 || std::abs(newXMax - xMax) > 1e-10) {
            customPlot->xAxis->setRange(newXMin, newXMax);
        }
    }

    // 5. 重绘
    customPlot->replot();
}

QVector<QColor> GenerateColors() {
    QVector<QColor> colors;
    colors << QColor(0, 0, 0)
        << QColor(0, 255, 0)
        << QColor(0, 0, 255)
        << QColor(255, 0, 0)
        << QColor(255, 255, 0)
        << QColor(0, 255, 255)
        << QColor(255, 165, 0)
        << QColor(100, 100, 100)
        << QColor(255, 0, 255)
        << QColor(165, 42, 42)
        << QColor(173, 216, 230)
        << QColor(144, 238, 144)
        << QColor(200, 200, 200)
        << QColor(128, 0, 128)
        << QColor(255, 250, 205)
        << QColor(255, 127, 120);

    return colors;
}

PassiveRadar::PassiveRadar(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    this->LoadStyleSheet("my.qss");
    ClearAction = new QAction("clear", this);
    connect(ClearAction, &QAction::triggered, [this]() {ui.textBrowser->clear(); });
    ui.textBrowser->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.textBrowser, &QTextEdit::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(ClearAction);
        menu.exec(ui.textBrowser->mapToGlobal(pos));
        });

    connect(ui.comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(doCurrentIndexChanged(int)));
    connect(ui.comboBox_CH, SIGNAL(currentIndexChanged(int)), &procThread, SLOT(GetCH(int)));
    connect(ui.graphicsView, SIGNAL(SendMessageSignal(QString)), this, SLOT(ReceiveMessageSlot(QString)));
    connect(this, SIGNAL(PlotTRSignal(double, double, double, double)), ui.graphicsView, SLOT(PlotTR(double, double, double, double)));

    connect(&procThread, &ProcThread::PlotEnu, this, &PassiveRadar::doPlotEnu);
    connect(&procThread, &ProcThread::PlotTarget, ui.graphicsView, &mymap::PlotItem);
    connect(&procThread, &ProcThread::PlotFFT, this, &PassiveRadar::doPlotFFT);
    connect(&procThread, &ProcThread::PlotPhase, this, &PassiveRadar::doPlotPhase);
    connect(&procThread, &ProcThread::ShowResult, this, &PassiveRadar::doShowResult);
    init();
    
}
void PassiveRadar::on_start_clicked()
{
    ui.graphicsView->ClearItems();
    ui.page_1->clearGraphs();
    ui.page_2->clearGraphs();
    ui.page_3->clearGraphs();
    ui.page_1->replot();
    ui.page_2->replot();
    ui.page_3->replot();

    uiData.graphidx = 0;
    ui.page_1->addGraph();

    for (int i = 0; i < MaxCH; ++i)
    {
        ui.page_2->addGraph();
        QPen pen(colors[i]);
        pen.setWidth(2);
        ui.page_2->graph(i)->setPen(pen);
        ui.page_2->graph(i)->setName("CH" + QString::number(i));
    }

    uiData.FilePath = ui.lineEdit->text().toLocal8Bit().data();
    uiData.mask_of_CH = ui.mask_of_CH->text().toULongLong(nullptr, 16);
    uiData.files.clear();
    GetFiles(uiData.FilePath, uiData.files);

	uiData.sr = 1000000 * ui.sr->text().toULongLong();
	uiData.fzero = 1000000 * ui.fzero->text().toULongLong();
	uiData.T = ui.T->text().toDouble();
	uiData.d = ui.d->text().toDouble();
	uiData.alphi = ui.alphi->text().toDouble();
	uiData.offset = ui.offset->text().toDouble();
	uiData.Fshift = 1000000 * ui.Fshift->text().toDouble();
    uiData.GPS_base[0] = ui.base0->text().toDouble();
    uiData.GPS_base[1] = ui.base1->text().toDouble();
    uiData.GPS_base[2] = ui.base2->text().toDouble();
    uiData.GPS_tower[0] = ui.tower0->text().toDouble();
    uiData.GPS_tower[1] = ui.tower1->text().toDouble();
    uiData.GPS_tower[2] = ui.tower2->text().toDouble();
    uiData.tshift = ui.tshift->text().toDouble();
    uiData.theta_ref = ui.theta_ref->text().toDouble();

    uiData.xyz[0] = uiData.xyz[1] = uiData.xyz[2] = 0;
    doPlotEnu(0.0, 0.0);
    WGS2ENU(uiData.GPS_tower, uiData.GPS_base, uiData.xyz);
    doPlotEnu(uiData.xyz[0], uiData.xyz[1]);
    PlotTRSignal(uiData.GPS_base[1], uiData.GPS_base[0], uiData.GPS_tower[1], uiData.GPS_tower[0]);
    uiData.len = uiData.T * uiData.sr;

    uiData.nlength = (GetLength(uiData.files[0], 0) - klength * uiData.len) / uiData.len;
    uiData.FT = ui.comboBox->currentIndex();
    //doShowResult("find device ...");
    procThread.init(uiData);
    procThread.start(uiData);
    //std::cout << QThread::currentThreadId << std::endl;
}
void PassiveRadar::on_stop_clicked()
{
    procThread.stop();
}



void PassiveRadar::init()
{
    x_.resize(FFTSize);
    y_.resize(FFTSize);
    LoadLastInput();
    ui.stackedWidget->setCurrentIndex(0);
    ui.keep->setCheckState(Qt::Unchecked);

    colors = GenerateColors();
    QFont font("Times New Roman", 11);
    
    ui.page_1->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    ui.page_1->xAxis2->setVisible(true);
    ui.page_1->xAxis2->setTickLabels(false);
    ui.page_1->yAxis2->setVisible(true);
    ui.page_1->yAxis2->setTickLabels(false);
    ui.page_1->xAxis->setTickLabelFont(font);
    ui.page_1->yAxis->setTickLabelFont(font);
    ui.page_1->xAxis->setLabelFont(font);
    ui.page_1->yAxis->setLabelFont(font);
    ui.page_1->xAxis->setLabel("Frequency/MHz");
    ui.page_1->yAxis->setLabel("PSD/dB");

    ui.page_2->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    ui.page_2->xAxis2->setVisible(true);
    ui.page_2->xAxis2->setTickLabels(false);
    ui.page_2->yAxis2->setVisible(true);
    ui.page_2->yAxis2->setTickLabels(false);
    ui.page_2->xAxis->setTickLabelFont(font);
    ui.page_2->yAxis->setTickLabelFont(font);
    ui.page_2->xAxis->setLabelFont(font);
    ui.page_2->yAxis->setLabelFont(font);
    ui.page_2->xAxis->setLabel("Times");
    ui.page_2->yAxis->setLabel("Phase/°");
    ui.page_2->legend->setVisible(1);
    ui.page_2->legend->setSelectableParts(QCPLegend::spItems);
    ui.page_2->legend->setFillOrder(QCPLayoutGrid::foColumnsFirst);
    ui.page_2->legend->setWrap(4);
    QFont newRomanFont("Times New Roman", 10);
    ui.page_2->legend->setFont(newRomanFont);


    ui.page_3->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    ui.page_3->xAxis2->setVisible(true);
    ui.page_3->xAxis2->setTickLabels(false);
    ui.page_3->yAxis2->setVisible(true);
    ui.page_3->yAxis2->setTickLabels(false);
    ui.page_3->xAxis->setTickLabelFont(font);
    ui.page_3->yAxis->setTickLabelFont(font);
    ui.page_3->xAxis->setLabelFont(font);
    ui.page_3->yAxis->setLabelFont(font);
    ui.page_3->xAxis->setLabel("East/m");
    ui.page_3->yAxis->setLabel("North/m");
    

    uiData.GPS_base[0] = ui.base0->text().toDouble();
    uiData.GPS_base[1] = ui.base1->text().toDouble();
    uiData.GPS_base[2] = ui.base2->text().toDouble();
    uiData.GPS_tower[0] = ui.tower0->text().toDouble();
    uiData.GPS_tower[1] = ui.tower1->text().toDouble();
    uiData.GPS_tower[2] = ui.tower2->text().toDouble();

    doPlotEnu(0.0, 0.0);
    WGS2ENU(uiData.GPS_tower, uiData.GPS_base, uiData.xyz);
    doPlotEnu(uiData.xyz[0], uiData.xyz[1]);
    PlotTRSignal(uiData.GPS_base[1], uiData.GPS_base[0], uiData.GPS_tower[1], uiData.GPS_tower[0]);
    
}
void PassiveRadar::doPlotFFT(double* x, double* y)
{
    for (int i = 0; i < FFTSize; ++i)
    {
        x_[i] = x[i];
        y_[i] = y[i];
    }
    ui.page_1->graph(0)->setData(x_, y_);
    ui.page_1->yAxis->setRange(30., 120.);
    ui.page_1->xAxis->setRange(x_.first(), x_.last());
    ui.page_1->replot();
}
void PassiveRadar::doPlotPhase(double x, double* y)
{

    ui.page_2->yAxis->setRange(-180., 240.);
    ui.page_2->xAxis->setRange(1, x + 1);

    for (int i = 0; i < MaxCH; ++i) {

        ui.page_2->graph(i)->addData(x, y[i]);
    }
    ui.page_2->replot();
}
void PassiveRadar::doPlotEnu(double x, double y)
{
    QVector<double> x_(1);
    QVector<double> y_(1);
    x_[0] = x;
    y_[0] = y;
    
    ui.page_3->legend->setVisible(1);
    ui.page_3->addGraph();
    
    QPen drawpen;
    if (uiData.graphidx > 1)
    {
        drawpen.setColor(Qt::red);
        drawpen.setWidth(5);
        drawpen.setCapStyle(Qt::RoundCap);
        ui.page_3->graph(uiData.graphidx)->setName("目标");
        if (uiData.graphidx > 2) { ui.page_3->legend->removeItem(ui.page_3->legend->itemCount() - 1); }
    }
    if (uiData.graphidx == 0)
    {
        drawpen.setColor(Qt::blue);
        drawpen.setWidth(8);
        ui.page_3->graph(0)->setName("接收站");
    }
    if (uiData.graphidx == 1)
    {
        drawpen.setColor(Qt::green);
        drawpen.setWidth(8);
        ui.page_3->graph(1)->setName("辐射源");
    }

    ui.page_3->graph(uiData.graphidx)->setPen(drawpen);
    
    ui.page_3->graph(uiData.graphidx)->setLineStyle(QCPGraph::lsNone);
    ui.page_3->graph(uiData.graphidx)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 2));

    ui.page_3->graph(uiData.graphidx)->setData(x_, y_);
    ui.page_3->graph(uiData.graphidx)->rescaleAxes(1);
    ui.page_3->replot();
    ++uiData.graphidx;
    enforceEqualAxesScale(ui.page_3);
}
void PassiveRadar::doShowResult(QString str)
{
    ui.textBrowser->append(str);
}
void PassiveRadar::doCurrentIndexChanged(int n)
{
    ui.stackedWidget->setCurrentIndex(n);
}
void PassiveRadar::on_choose_clicked()
{
    QString Qstr = QFileDialog::getExistingDirectory(this, QStringLiteral("Open Directory"), QCoreApplication::applicationDirPath());
    ui.lineEdit->setText(Qstr);//.replace("/", "\\")
}
void PassiveRadar::LoadStyleSheet(const QString& styleSheetFile)
{
    QFile file(styleSheetFile);
    file.open(QFile::ReadOnly);
    if (file.isOpen())
    {
        QString styleSheet = this->styleSheet();
        styleSheet += QLatin1String(file.readAll());
        this->setStyleSheet(styleSheet);
        file.close();
        //QMessageBox::information(this, "tip", "qss file already add");
    }
    else
    {
        QMessageBox::information(this, "tip", "cannot find qss file");
    }
}
void PassiveRadar::LoadLastInput()
{
    settings = new QSettings("config.ini", QSettings::IniFormat, this);
    settings->beginGroup("PassiveRadar");
    ui.lineEdit->setText(settings->value("lineEdit", "D:\\wujiaqiao\\data_").toString());

    ui.sr->setText(settings->value("sr", "10").toString());
    ui.fzero->setText(settings->value("fzero", "626").toString());
    ui.T->setText(settings->value("T", "1").toString());
    ui.mask_of_CH->setText(settings->value("mask_of_CH", "FFFF").toString());
    ui.base0->setText(settings->value("base0", "30.728330").toString());
    ui.base1->setText(settings->value("base1", "120.593812").toString());
    ui.base2->setText(settings->value("base2", "5").toString());
    ui.tower0->setText(settings->value("tower0", "30.774399").toString());
    ui.tower1->setText(settings->value("tower1", "120.746142").toString());
    ui.tower2->setText(settings->value("tower2", "100").toString());
    ui.tshift->setText(settings->value("tshift", "0").toString());
    ui.theta_ref->setText(settings->value("theta_ref", "76").toString());
    ui.d->setText(settings->value("d", "0.5").toString());
    ui.offset->setText(settings->value("offset", "13").toString());
    ui.Fshift->setText(settings->value("Fshift", "0").toString());
    ui.alphi->setText(settings->value("alphi", "0").toString());

    settings->endGroup();
}
void PassiveRadar::SaveLastInput()
{
    settings->beginGroup("PassiveRadar");
    settings->setValue("lineEdit", ui.lineEdit->text());
    settings->setValue("sr", ui.sr->text());
    settings->setValue("fzero", ui.fzero->text());
    settings->setValue("T", ui.T->text());
    settings->setValue("mask_of_CH", ui.mask_of_CH->text());
    settings->setValue("base0", ui.base0->text());
    settings->setValue("base1", ui.base1->text());
    settings->setValue("base2", ui.base2->text());
    settings->setValue("tower0", ui.tower0->text());
    settings->setValue("tower1", ui.tower1->text());
    settings->setValue("tower2", ui.tower2->text());
    settings->setValue("tshift", ui.tshift->text());
    settings->setValue("theta_ref", ui.theta_ref->text());
    settings->setValue("d", ui.d->text());
    settings->setValue("offset", ui.offset->text());
    settings->setValue("Fshift", ui.Fshift->text());
    settings->setValue("alphi", ui.alphi->text());

    settings->endGroup();
}
void PassiveRadar::ReceiveMessageSlot(QString Position)
{
    ui.position->setText(Position);
}
PassiveRadar::~PassiveRadar()
{
    SaveLastInput();
    delete settings;
}

