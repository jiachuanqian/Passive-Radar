#pragma once

#include "typedefs.h"
#include <QThread>
#include "CudaApi.cuh"

class ProcThread : public QObject
{
	Q_OBJECT
public:
	std::unique_ptr<std::thread> thread_;
	std::atomic_bool StopFlag;
	mutable std::mutex mutex_;
	State state_;
	int ch{};
	std::unique_ptr<fftw_complex[], decltype(&fftw_free)> fft_in; //<fftw_complex[], void(*)(void*)>
	std::unique_ptr<fftw_complex[], decltype(&fftw_free)> fft_out;
	std::unique_ptr<fftw_plan_s, decltype(&fftw_destroy_plan)> fft_plan;
	double fftx[FFTSize];
	double ffty[FFTSize];
	uint temp[MaxCH][Nphase]{};
	std::vector<double> vectors[MaxCH - 1];
	GPUData GD;

	explicit ProcThread(QObject* parent = nullptr);
	virtual ~ProcThread();

	void init(UiData uiData);
	void start(UiData uiData);
	void stop();
	void process(UiData uiData);
	void fftshow(UiData uiData);
	void mphase(UiData uiData);
	void detect(UiData uiData);
	void location(UiData& uiData, GPUData GD);
	State state() const;

signals:
	void PlotEnu(double x, double y);
	void PlotTarget(double x, double y);
	void PlotFFT(double* x, double* y);
	void PlotPhase(double x, double* y);
	void ShowResult(QString str);
public slots:
	void GetCH(int CH);
};
