
#include "CudaApi.cuh"

void MakeRef(float theta_ref, UiData uiData, GPUData GD)
{
	cuComplex h_w[MaxCH]{};
	cuComplex* d_w{};
	CUDA_CHECK(cudaMalloc(&d_w, sizeof(cuComplex) * MaxCH));
	double k = 2.0 * pi * uiData.d * sin(theta_ref * pi / 180.0f) / (double(c) / uiData.fzero);
	for (size_t i = 0; i < MaxCH; i++) {
		h_w[i] = make_cuComplex(cos(k * i), -sin(k * i));
	}
	CUDA_CHECK(cudaMemcpy(d_w, h_w, MaxCH * sizeof(cuComplex), cudaMemcpyHostToDevice));
	
	CUBLAS_CHECK(cublasCgemm(GD.handle_cublas, CUBLAS_OP_N, CUBLAS_OP_T,
		1, (1 + klength) * uiData.len, MaxCH,
		&GD.alpha,
		d_w, 1,
		GD.d_data, (1 + klength) * uiData.len,
		&GD.beta,
		GD.refk, 1));

	if (d_w) { cudaFree(d_w); d_w = nullptr; }
}

void LinearSolverLU(cusolverDnHandle_t handle_LU, GPUData GD)
{
	int bufferSize{};
	cuComplex* buffer{};
	
	CUSOLVER_CHECK(cusolverDnCgetrf_bufferSize(handle_LU, MaxCH, MaxCH, GD.d_Rx, MaxCH, &bufferSize));
	CUDA_CHECK(cudaMalloc(&buffer, sizeof(cuComplex) * bufferSize));
	CUSOLVER_CHECK(cusolverDnCgetrf(handle_LU, MaxCH, MaxCH, GD.d_Rx, MaxCH, buffer, GD.ipiv, GD.info));

	if (buffer) { cudaFree(buffer); buffer = nullptr; }
}

void ComputeWeight(float ll, UiData uiData, GPUData GD)
{
	double k = 2.0 * pi * uiData.d * sin(ll * pi / 180.0f) / (double(c) / uiData.fzero);

	for (size_t i = 0; i < MaxCH; i++) {
		GD.h_s0[i] = make_cuComplex(cos(k * i), -sin(k * i));
	}
	CUDA_CHECK(cudaMemcpy(GD.d_s0, GD.h_s0, MaxCH * sizeof(cuComplex), cudaMemcpyHostToDevice));
	CUDA_CHECK(cudaMemcpy(GD.d_s1, GD.d_s0, MaxCH * sizeof(cuComplex), cudaMemcpyDeviceToDevice));
	// cacl (Rx\s0)
	CUSOLVER_CHECK(cusolverDnCgetrs(GD.handle_cusolver, CUBLAS_OP_N, MaxCH, 1, GD.d_Rx, MaxCH, GD.ipiv, GD.d_s1, MaxCH, GD.info));
	// cacl (s0'*(Rx\s0))
	CUBLAS_CHECK(cublasCdotc(GD.handle_cublas, MaxCH, GD.d_s0, 1, GD.d_s1, 1, &GD.denominator));
	// cacl weight = (Rx\s0)
	CUDA_CHECK(cudaMemcpy(GD.weight, GD.d_s1, MaxCH * sizeof(cuFloatComplex), cudaMemcpyDeviceToDevice));
	float denom_norm = GD.denominator.x * GD.denominator.x + GD.denominator.y * GD.denominator.y;
	cuComplex inv_denominator = make_cuComplex(
		GD.denominator.x / denom_norm,
		-GD.denominator.y / denom_norm );
	// weight = (Rx\s0) / (s0'*(Rx\s0))
	CUBLAS_CHECK(cublasCscal(GD.handle_cublas, MaxCH, &inv_denominator, GD.weight, 1));
}


void reshape(GPUData GD)
{
	kernelreshape <<< GD.blocksPerGrid, GD.threadsPerBlock >>> (GD);
}
__global__ void kernelreshape(GPUData GD)
{
	int tid = blockIdx.x * blockDim.x + threadIdx.x;

	if (tid >= GD.Nrd) return;

	int idx = tid / GD.Nplus;
	int idy = tid % GD.Nplus;

	if (idx < GD.M) {
		if (idy < GD.N) {
			GD.ref[tid] = GD.refk[idy + idx * GD.N];
			GD.echo[tid] = GD.echok[idy + idx * GD.N];
		}
		else if (idy < GD.Nplus) {
			GD.ref[tid] = make_cuComplex(0.0f, 0.0f);
			GD.echo[tid] = GD.echok[idy + idx * GD.N];
		}
	}
}

void fftShift(GPUData GD, int dir)
{
	if (dir == 0) {
		kernelfftShift<0> <<< GD.blocksPerGrid, GD.threadsPerBlock >>> (GD);
	}
	else {
		kernelfftShift<1> <<< GD.blocksPerGrid, GD.threadsPerBlock >>> (GD);
	}
}

template <int dir>
__global__ void kernelfftShift(GPUData GD)
{
	int tid = blockIdx.x * blockDim.x + threadIdx.x;
	if (tid >= GD.Nrd) return;

	int row = tid / GD.Nplus;
	int col = tid % GD.Nplus;

	int id = (dir == 0) ? col : row;
	float sign = (id & 1) ? -1.0f : 1.0f;

	GD.echo[tid].x *= sign;
	GD.echo[tid].y *= sign;
	GD.ref[tid].x *= sign;
	GD.ref[tid].y *= sign;
}


void ConjMul(GPUData GD)
{
	kernelConjMul <<< GD.blocksPerGrid, GD.threadsPerBlock >>> (GD);
}

__global__ void kernelConjMul(GPUData GD)
{
	int tid = blockIdx.x * blockDim.x + threadIdx.x;

	if (tid >= GD.Nrd) return;

	cuComplex e = GD.echo[tid];
	cuComplex r = GD.ref[tid];

	cuComplex temp{};

	temp.x = (e.x * r.x) + (e.y * r.y);
	temp.y = (e.y * r.x) - (e.x * r.y)  ;

	float invN = 1.0f / GD.Nplus;
	GD.echo[tid] = make_cuComplex(temp.x * invN, temp.y * invN);
}


void ComPhase(GPUData GD, UiData uiData) {
	dim3 block(1024);
	dim3 grid((int(MaxCH * (1 + klength) * uiData.len) + block.x - 1) / block.x);
	kernelComPhase <<< grid, block >>> (GD, uiData);
}

__global__ void  kernelComPhase( GPUData GD, UiData uiData) {
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	int total = MaxCH * (1 + klength) * uiData.len;

	if (idx >= total) return;

	int ch = idx / int((1 + klength) * uiData.len);

	int val = GD.d_dataint[idx];
	float i = int16_t(val & 0xFFFF);
	float q = int16_t(val >> 16);;

	float cosp = cosf(-GD.d_phase[ch]*pi/180);
	float sinp = sinf(-GD.d_phase[ch]*pi/180);

	float real = i * cosp - q * sinp;
	float imag = i * sinp + q * cosp;

	GD.d_data[idx] = make_cuComplex(real, imag);
}