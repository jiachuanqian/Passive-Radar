#pragma once


#include "ui_PassiveRadar.h"
#include "typedefs.h"
#include "ProcThread.h"

QVector<QColor> GenerateColors();
extern ProcThread procThread;

class PassiveRadar : public QWidget
{
    Q_OBJECT

public:
    UiData uiData;
    QAction* ClearAction;
    QSettings* settings;
    QVector<QColor> colors;
    QVector<double> x_, y_;


    PassiveRadar(QWidget *parent = nullptr);
    
    void init();
    void LoadStyleSheet(const QString& styleSheetFile);
    void LoadLastInput();
    void SaveLastInput();
    ~PassiveRadar();
signals:
    void PlotTRSignal(double x, double y, double xx, double yy);
    void PlotEnu(double x, double y);
private slots:
    void on_start_clicked();
    void on_stop_clicked();
    void on_choose_clicked();    
    void doCurrentIndexChanged(int n);    
    void ReceiveMessageSlot(QString Position);
    void doPlotEnu(double x, double y);
    void doPlotFFT(double* x, double* y);
    void doPlotPhase(double x, double* y);
    void doShowResult(QString str);
    //void GetCH(int CH);
private:
    Ui::PassiveRadarClass ui;
};

