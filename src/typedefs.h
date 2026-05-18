#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <fftw3.h>
#include <complex>
#include <vector>
#include <map>
//#include <atomic>
//#include <thread>
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif

struct DetectionResult {
    size_t beam_idx{};
    size_t idx{};
    float snr{};
};


void ENU2WGS(double GPS_base[3], double* xyz);
void WGS2ENU(double GPS_tower[3], double GPS_base[3], double* xyz);
size_t GetLength(std::string filename, int n);
void GetFiles(std::string path, std::vector<std::string>& files);
void ReadFile(std::string filename, void* data, int size, size_t start);
void ReadPhaseFile(float* array, int size, const std::string & filename);
//void cfar(std::complex<float>*out, size_t idx, size_t TM, size_t TN);
float cfar(std::complex<float>*out, size_t n_beams, size_t TM, size_t TN, size_t & beam_idx, size_t & idx);
std::vector<DetectionResult> cfar_(std::complex<float>* out, size_t n_beams, size_t TM, size_t TN, float threshold_db);
void save_results_to_csv(const std::string& filename,
    int call_number,
    const std::vector<DetectionResult>& results,
    bool append_header = true);

constexpr size_t FFTSize = 1024;
constexpr size_t c = 300000000;
constexpr size_t rate = 7560000;
constexpr size_t MaxCH = 16;
constexpr size_t Nphase = 100000;
constexpr double klength = 0.02;
constexpr double pi = 3.141592653589793;
constexpr double a = 6378137.0;
constexpr double e = 0.081819190842622;



struct UiData {
	std::string FilePath;
	std::vector<std::string> files;
    double GPS_base[3]{}, GPS_tower[3]{}, xyz[3]{}, target[3]{};
    double T{}, d{}, alphi{}, offset{}, theta{};
    size_t sr{}, fzero{}, mask_of_CH{}, graphidx{}, FT{}, len{}, nlength{}, Fshift{}, CH{}, idx{}, tshift{};
    float CHphase[MaxCH]{};
    float theta_ref{}, start_deg{ -8.0f }, end_deg{ 8.0f }, step{ 1.0f };
};

enum State
{
	Idle,
	Stopped,
	Running,
	Paused
};



