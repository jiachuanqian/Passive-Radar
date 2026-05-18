#pragma once

#include "typedefs.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <cuComplex.h>
#include <cufft.h>


#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA Error: " << cudaGetErrorString(error) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

#define CUBLAS_CHECK(call) \
    do { \
        cublasStatus_t status = call; \
        if (status != CUBLAS_STATUS_SUCCESS) { \
            fprintf(stderr, "cuBLAS Error at %s:%d: %s\n", \
                    __FILE__, __LINE__, cublasGetStatusString(status)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

#define CUSOLVER_CHECK(call) \
    do { \
        cusolverStatus_t status = call; \
        if (status != CUSOLVER_STATUS_SUCCESS) { \
            std::cerr << "cuSolver error at " << __FILE__ << ":" << __LINE__ << " - " << status << std::endl; \
            exit(1); \
        } \
    } while(0)

struct GPUData {
    cublasHandle_t handle_cublas{};
    cusolverDnHandle_t handle_cusolver{};
    cufftHandle plan_FFT1{}, plan_FFT2{};
    int* h_data{}, * d_dataint{};
    int* info{}, * ipiv{};
    int  threadsPerBlock{ 1024 }, blocksPerGrid{};
    int n1[1]{}, inembed[1]{}, onembed[1]{}, n2[1]{};
    std::complex<float>* h_RCC{};
    cuComplex alpha = { 1,0 }, beta = { 0,0 }, h_s0[MaxCH]{}, denominator{};
    cuComplex* d_data{}, * d_Rx{}, * d_s0{}, * d_s1{}, * weight{}, * refk{}, * echok{}, * echo{}, * ref{}, * RC{}, * RCC{};
    float* d_phase{};
    size_t TM{}, TN{}, NT{}, M{}, N{}, Nplus{}, Nrd{}, num_points{};
};



void LinearSolverLU(cusolverDnHandle_t handle_LU, GPUData GD);
void ComputeWeight(float ll, UiData uiData, GPUData GD);
void reshape(GPUData GD);
void MakeRef(float theta_ref, UiData uiData, GPUData GD);
__global__ void kernelreshape(GPUData GD);
void fftShift(GPUData GD, int dir);
template <int dir>
__global__ void kernelfftShift(GPUData GD);
void ConjMul(GPUData GD);
__global__ void kernelConjMul(GPUData GD);
void ComPhase(GPUData GD, UiData uiData);
__global__ void  kernelComPhase(GPUData GD, UiData uiData);