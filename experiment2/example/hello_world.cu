#include<iostream>
#include<cuda_runtime.h>

using namespace std;

class name
{
    private:
    int x;
    public:
    void hello()
    {

    }
    int y;
};

__global__ void test108()
{
    printf("\nhello world from device");
}

int main()
{
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp,0);
    cout<<"Device Name: "<<deviceProp.name<<", sm_count: "<<deviceProp.multiProcessorCount<<endl;
    
    int cudaVersion;
    cudaError_t cudaStatus = cudaRuntimeGetVersion(&cudaVersion);
    if(cudaStatus == cudaSuccess) 
    {
        cout << "CUDA Version: " << cudaVersion << std::endl;
    } 
    else 
    {
        cerr << "Error getting CUDA version: " << cudaGetErrorString(cudaStatus) << std::endl;
    }
    
    int blocks=1,threads=512;
    test108<<<blocks,threads>>>();
    cudaDeviceSynchronize();

    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) 
    {
        cout<<"CUDA error: "<< cudaGetErrorString(error);
    }

    return 0;
}
