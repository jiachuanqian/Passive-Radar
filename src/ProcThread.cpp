#include "ProcThread.h"


template <typename T>
void saveFile(T* data, int size, int ll, int i)
{
    std::string dirPath = "./outdata/" + std::to_string(i);

    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
    }

    std::string filename = dirPath + "/out_" + std::to_string(ll) + ".data";
    
    std::ofstream outfile(filename, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(reinterpret_cast<const char*>(data), size * sizeof(T));
        outfile.close();
    }
    else {
        std::cout << "unable to save file" << std::endl;
    }
}


ProcThread::ProcThread(QObject* parent)
    : QObject(parent), thread_(nullptr), StopFlag(false), state_(Idle),
    fft_in(fftw_alloc_complex(FFTSize), fftw_free),
    fft_out(fftw_alloc_complex(FFTSize), fftw_free),
    fft_plan(fftw_plan_dft_1d(FFTSize, fft_in.get(), fft_out.get(), FFTW_FORWARD, FFTW_ESTIMATE), fftw_destroy_plan)
{
    cublasCreate(&GD.handle_cublas);
    cusolverDnCreate(&GD.handle_cusolver);

}
void ProcThread::init(UiData uiData)
{
    GD.TM = 2 * 380 * uiData.T + 1;
    GD.TN = 1000;
    GD.NT = uiData.len;
    GD.N = 4000;
    GD.M = GD.NT / GD.N;
    GD.Nplus = GD.N + GD.TN;
    GD.Nrd = GD.M * GD.Nplus;
    GD.num_points = static_cast<size_t>((uiData.end_deg - uiData.start_deg) / uiData.step) + 1;
    


    GD.blocksPerGrid = (GD.Nrd + GD.threadsPerBlock - 1) / GD.threadsPerBlock;
    GD.n1[0] = GD.Nplus;
    GD.n2[0] = GD.M;
    GD.inembed[0] = GD.M * GD.Nplus;
    GD.onembed[0] = GD.M * GD.Nplus;
    cufftPlanMany(&GD.plan_FFT1, 1, GD.n1, GD.inembed, 1, GD.Nplus, GD.onembed, 1, GD.Nplus, CUFFT_C2C, GD.M);
    cufftPlanMany(&GD.plan_FFT2, 1, GD.n2, GD.inembed, GD.Nplus, 1, GD.onembed, GD.Nplus, 1, CUFFT_C2C, GD.Nplus);
    ReadPhaseFile(uiData.CHphase, MaxCH, "phase.txt");

    CUDA_CHECK(cudaFreeHost(GD.h_data));
    CUDA_CHECK(cudaFreeHost(GD.h_RCC));
    CUDA_CHECK(cudaFree(GD.d_dataint));
    CUDA_CHECK(cudaFree(GD.d_data));
    CUDA_CHECK(cudaFree(GD.d_Rx));
    CUDA_CHECK(cudaFree(GD.info));
    CUDA_CHECK(cudaFree(GD.ipiv));
    CUDA_CHECK(cudaFree(GD.d_s0));
    CUDA_CHECK(cudaFree(GD.d_s1));
    CUDA_CHECK(cudaFree(GD.weight));
    CUDA_CHECK(cudaFree(GD.refk));
    CUDA_CHECK(cudaFree(GD.echok));
    CUDA_CHECK(cudaFree(GD.echo));
    CUDA_CHECK(cudaFree(GD.ref));
    CUDA_CHECK(cudaFree(GD.RC));
    CUDA_CHECK(cudaFree(GD.RCC));
    CUDA_CHECK(cudaFree(GD.d_phase));

    CUDA_CHECK(cudaMallocHost(&GD.h_RCC, GD.TN * GD.TM * GD.num_points * sizeof(std::complex<float>)));
    CUDA_CHECK(cudaMallocHost(&GD.h_data, MaxCH * (1 + klength) * uiData.len * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&GD.d_dataint, MaxCH * (1 + klength) * uiData.len * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&GD.d_data, MaxCH * (1 + klength) * uiData.len * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.d_Rx, MaxCH * MaxCH * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.info, sizeof(int)));
    CUDA_CHECK(cudaMalloc(&GD.ipiv, MaxCH * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&GD.d_s0, MaxCH * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.d_s1, MaxCH * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.weight, MaxCH * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.refk, (1 + klength) * uiData.len * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.echok, (1 + klength) * uiData.len * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.echo, GD.Nrd * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.ref, GD.Nrd * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.RC, GD.TM * GD.Nplus * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.RCC, GD.TM * GD.TN * sizeof(cuComplex)));
    CUDA_CHECK(cudaMalloc(&GD.d_phase, MaxCH * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(GD.d_phase, uiData.CHphase, MaxCH * sizeof(float), cudaMemcpyHostToDevice));
}
void ProcThread::start(UiData uiData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == Running) {
        stop();
    }
    StopFlag = false;
    state_ = Running;

    thread_ = std::make_unique<std::thread>([this, uiData]() {
        process(uiData);
        });

    //    thread_ = std::make_unique<std::thread>([=]() {this->process(a); });

}
void ProcThread::stop()
{
    StopFlag = true;
    if (thread_ && thread_->joinable()) {
        thread_->join(); 
        thread_.reset(); 
    }       
    state_ = Stopped;
}

void ProcThread::process(UiData uiData)
{  
    switch (uiData.FT) {
    case 0:fftshow(uiData); break;
    case 1:mphase(uiData); break;
    case 2:
    case 3:detect(uiData); break;
    }
}
void ProcThread::GetCH(int CH)
{
    ch = CH;
}
void ProcThread::fftshow(UiData uiData)
{
    uint temp[FFTSize]{};
    for (int i = 0; i < FFTSize; ++i)
    {
        fftx[i] = uiData.sr / 1e6 * (-0.5 + 1.0 * i / FFTSize) + uiData.fzero / 1e6;
        ffty[i] = 0.0;
    }
    for (size_t i = 0; i < uiData.nlength; ++i)//
    {
        ReadFile(uiData.files[ch], temp, FFTSize * sizeof(int), i * uiData.sr * sizeof(int));
        for (int j = 0; j < FFTSize; ++j)
        {
            fft_in[j][0] = static_cast<int16_t>(temp[j] & 0xFFFF);
            fft_in[j][1] = static_cast<int16_t>(temp[j] >> 16);
        }
        fftw_execute(fft_plan.get());
        for (int j = 0; j < FFTSize / 2; ++j)
        {
            ffty[j] = 10.0 * log10(fft_out[FFTSize / 2 + j][0] * fft_out[FFTSize / 2 + j][0] + fft_out[FFTSize / 2 + j][1] * fft_out[FFTSize / 2 + j][1]);
            ffty[FFTSize / 2 + j] = 10.0 * log10(fft_out[j][0] * fft_out[j][0] + fft_out[j][1] * fft_out[j][1]);
        }
        if (StopFlag) { break; }
        else {
            emit PlotFFT(fftx, ffty);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); } 
        //std::cout << i;
    }
}
void ProcThread::mphase(UiData uiData)
{    
    for (size_t i = 0; i < uiData.nlength; ++i)
    {
        double angle[MaxCH]{};
        ReadFile(uiData.files[0], temp[0], Nphase * sizeof(int), i * uiData.sr * sizeof(int));
        for (size_t j = 1; j < MaxCH; ++j)
        {
            ReadFile(uiData.files[j], temp[j], Nphase * sizeof(int), i * uiData.sr * sizeof(int));

                double temp_real = 0;
                double temp_imag = 0;
                for (size_t k = 0; k < Nphase; ++k)
                {
                    temp_real = temp_real + double(static_cast<int16_t>(temp[0][k] & 0xFFFF) * static_cast<int16_t>(temp[j][k] & 0xFFFF) + static_cast<int16_t>(temp[0][k] >> 16) * static_cast<int16_t>(temp[j][k] >> 16));
                    temp_imag = temp_imag + double(static_cast<int16_t>(temp[0][k] & 0xFFFF) * static_cast<int16_t>(temp[j][k] >> 16) - static_cast<int16_t>(temp[0][k] >> 16) * static_cast<int16_t>(temp[j][k] & 0xFFFF));
                }
                angle[j] = atan2(temp_imag, temp_real) * 180 / pi;
                vectors[j-1].push_back(angle[i]);
            
        }
        if (StopFlag) { break; }
        else {
            emit PlotPhase(i, angle);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        //emit ShowResult("find device ...");
    }
}
void ProcThread::detect(UiData uiData)
{
    //std::complex<float> *out = new std::complex<float>[GD.Nrd] {};
    emit ShowResult("开始处理" "\n" "-------------------------------------");
    for (size_t i = uiData.tshift; i < uiData.nlength; ++i)
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        for (size_t j = 0; j < MaxCH; ++j)
        {
            ReadFile(uiData.files[j], GD.h_data + uint(j * (1 + klength) * uiData.len), (1 + klength) * uiData.len * sizeof(int), i * uiData.len * sizeof(int));
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        CUDA_CHECK(cudaMemcpy(GD.d_dataint, GD.h_data, MaxCH * (1 + klength) * uiData.len * sizeof(int), cudaMemcpyHostToDevice));
        ComPhase(GD, uiData);

        CUBLAS_CHECK(cublasCgemm(GD.handle_cublas, CUBLAS_OP_C, CUBLAS_OP_N,
            MaxCH, MaxCH, (1 + klength) * uiData.len, 
            &GD.alpha,
            GD.d_data, (1 + klength) * uiData.len,
            GD.d_data, (1 + klength) * uiData.len, 
            &GD.beta, 
            GD.d_Rx, MaxCH));

        LinearSolverLU(GD.handle_cusolver, GD);

        for (float ll = uiData.start_deg; ll <= uiData.end_deg; ll += uiData.step)
        {

            ComputeWeight(ll, uiData, GD);

            CUBLAS_CHECK(cublasCgemm(GD.handle_cublas, CUBLAS_OP_N, CUBLAS_OP_T,
                1, (1 + klength) * uiData.len, MaxCH,
                &GD.alpha,
                GD.weight, 1,
                GD.d_data, (1 + klength) * uiData.len,
                &GD.beta,
                GD.echok, 1));
            if (0) { CUDA_CHECK(cudaMemcpy(GD.refk, GD.d_data, (1 + klength) * uiData.len * sizeof(cuComplex), cudaMemcpyDeviceToDevice)); }
            else { MakeRef(uiData.theta_ref, uiData, GD); }; //-147.0f  76.0f
            
            reshape(GD);
            fftShift(GD, 0);

            //cudaMemcpy(out, GD.ref, GD.Nrd * sizeof(cuComplex), cudaMemcpyDeviceToHost);

            cufftExecC2C(GD.plan_FFT1, GD.ref, GD.ref, CUFFT_FORWARD);
            cufftExecC2C(GD.plan_FFT1, GD.echo, GD.echo, CUFFT_FORWARD);
            ConjMul(GD);
            cufftExecC2C(GD.plan_FFT1, GD.echo, GD.echo, CUFFT_INVERSE);
            fftShift(GD, 1);
            cufftExecC2C(GD.plan_FFT2, GD.echo, GD.echo, CUFFT_FORWARD);
            CUDA_CHECK(cudaMemcpy(GD.RC, GD.echo + (GD.M - GD.TM + 1) / 2 * GD.Nplus, GD.TM * GD.Nplus * sizeof(cuComplex), cudaMemcpyDeviceToDevice));
            cublasGetMatrix(GD.TN, GD.TM, sizeof(cuComplex), GD.RC, GD.Nplus, GD.RCC, GD.TN);
           
            CUDA_CHECK(cudaMemcpy(GD.h_RCC + size_t((ll - uiData.start_deg) / uiData.step) * GD.TN * GD.TM, GD.RCC, GD.TN * GD.TM * sizeof(cuComplex), cudaMemcpyDeviceToHost));
            saveFile(GD.h_RCC + size_t((ll - uiData.start_deg) / uiData.step) * GD.TN * GD.TM, GD.TM * GD.TN, ll, i);
        }
        size_t beam_idx{};
        auto aaa = cfar(GD.h_RCC, GD.num_points, GD.TM, GD.TN, beam_idx, uiData.idx);
        // auto aaa = cfar_(GD.h_RCC, GD.num_points, GD.TM, GD.TN, uiData.offset);
        //save_results_to_csv("detection_results.csv", i, aaa, 1);

        auto t3 = std::chrono::high_resolution_clock::now();
        std::cout << " time: " << i << " ";
        std::cout << " read: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms " ;//<< std::endl
        std::cout << " proc: " << std::chrono::duration<double, std::milli>(t3 - t2).count() << " ms " << std::endl;//<< std::endl

        if (StopFlag) { break; }
        else {
            emit ShowResult("第" + QString::number(i) + "秒");
            //uiData.idx = uiData.idx;
            uiData.theta = beam_idx * uiData.step + uiData.start_deg;
            //std::cout << (uiData.idx % GD.TN) << std::endl;
            location(uiData, GD);
            emit ShowResult("SNR:" + QString::number(aaa, 'f', 2) + "dB" + "\n");
            emit PlotEnu(uiData.target[0], uiData.target[1]);
            ENU2WGS(uiData.GPS_base, uiData.target);
            emit PlotTarget(uiData.target[0], uiData.target[1]);           
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        //system("pause");
    }
    emit ShowResult("-------------------------------------" "\n" "处理完成");
}


ProcThread::~ProcThread()
{
    stop();
}
State ProcThread::state() const 
{ 
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}
void ProcThread::location(UiData& uiData, GPUData GD)
{
    double r0 = sqrt(uiData.xyz[0] * uiData.xyz[0] + uiData.xyz[1] * uiData.xyz[1]);
    double R = static_cast<double>(c) / (uiData.sr) * (uiData.idx % GD.TN);
    double r = R * (2 * r0 + R) / (2 * (r0 + R) - 2 * cos((uiData.alphi - uiData.theta) * pi / 180) * uiData.xyz[0] - 2 * sin((uiData.alphi - uiData.theta) * pi / 180) * uiData.xyz[1]);
    uiData.target[0] = r * cos((uiData.alphi - uiData.theta) * pi / 180);
    uiData.target[1] = r * sin((uiData.alphi - uiData.theta) * pi / 180);
    emit ShowResult("X:" + QString::number(uiData.target[0], 'f', 0) + "m,Y:" + QString::number(uiData.target[1], 'f', 0) + "m,Fd:" + QString::number(int(uiData.idx / GD.TN - GD.TM/2)) + "Hz");
}