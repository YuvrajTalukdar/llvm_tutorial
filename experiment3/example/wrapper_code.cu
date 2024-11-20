/**
*   CS6023: GPU Programming 
*   Assignment 1
*   
*   Please don't change any existing code in this file.
*
*   You can add your code whereever needed. Please add necessary memory APIs
*   for your implementation. Use cudaFree() to free up memory as soon as you're
*   done with an allocation. This will ensure that you don't run out of memory 
*   while running large test cases. Use the minimum required memory for your 
*   implementation. DO NOT change the kernel configuration parameters.
*/

#include <chrono>
#include <fstream>
#include <iostream>
#include <cuda.h>

using std::cin;
using std::cout;

struct dim6_index
{
    long int block_x,block_y,block_z;
    long int thread_x,thread_y,thread_z;
};

void f00109()
{

}
__host__ __device__
void foo108(int x)
{
    printf("%d",x);
}

__device__
long int get_id(dim6_index dim_index,dim3 block_dim,dim3 grid_dim,long int required_itteration,long int ittr_index)
{
    long int thread_id=blockDim.y*block_dim.x*required_itteration*dim_index.thread_z+
                        block_dim.x*required_itteration*dim_index.thread_y+
                        required_itteration*dim_index.thread_x+
                        ittr_index;
    long int block_id=grid_dim.x*grid_dim.y*dim_index.block_z+
                        grid_dim.x*dim_index.block_y+
                        dim_index.block_x;
    long int id=(block_dim.x*block_dim.y*block_dim.z*required_itteration)*block_id+thread_id;
    //printf("\nthreadId: %ld block_id: %ld id: %ld",thread_id,block_id,id);
    return id;
}

__device__
long int get_id(dim6_index dim_index,dim3 block_dim,dim3 grid_dim)
{
    long int thread_id=blockDim.y*block_dim.x*dim_index.thread_z+
                        block_dim.x*dim_index.thread_y+
                        dim_index.thread_x;
    long int block_id=grid_dim.x*grid_dim.y*dim_index.block_z+
                        grid_dim.x*dim_index.block_y+
                        dim_index.block_x;
    long int id=(block_dim.x*block_dim.y*block_dim.z)*block_id+thread_id;
    //printf("\nthreadId: %ld block_id: %ld id: %ld",thread_id,block_id,id);
    return id;
}

//divident=divisor*quotient+remainder
__device__
dim6_index get_dim6(long int id,dim3 block_dim,struct dim3 grid_dim)
{
    struct dim6_index dim_index;
    dim_index.block_z = id / (grid_dim.y * grid_dim.x * block_dim.z * block_dim.y * block_dim.x);
    dim_index.block_y = (id % (grid_dim.y * grid_dim.x * block_dim.z * block_dim.y * block_dim.x)) / (grid_dim.x * block_dim.z * block_dim.y * block_dim.x);
    dim_index.block_x = (id % (grid_dim.x * block_dim.z * block_dim.y * block_dim.x)) / (block_dim.z * block_dim.y * block_dim.x);
    dim_index.thread_z = (id % (block_dim.z * block_dim.y * block_dim.x)) / (block_dim.y * block_dim.x);
    dim_index.thread_y = (id % (block_dim.y * block_dim.x)) / block_dim.x;
    dim_index.thread_x = id % block_dim.x;
    return dim_index;
}

__device__
void calc_loop_itteration(dim3 gridDim_original,dim3 blockDim_original,long int &quotient,long int &remainder)
{
    quotient=(blockDim_original.x*blockDim_original.y*blockDim_original.z*gridDim_original.x*gridDim_original.y*gridDim_original.z) / 
            (blockDim.x*blockDim.y*blockDim.z*gridDim.x*gridDim.y*gridDim.z);
    remainder=(blockDim_original.x*blockDim_original.y*blockDim_original.z*gridDim_original.x*gridDim_original.y*gridDim_original.z) %
            (blockDim.x*blockDim.y*blockDim.z*gridDim.x*gridDim.y*gridDim.z);
}

__device__
void calc_loop_itteration(long int new_dim,long int original_dim,long int &required_itteration,long int &remainder)
{
    required_itteration=0;
    if(new_dim<original_dim)
    {   
        required_itteration=original_dim/new_dim;
        remainder=original_dim%new_dim;
    }
    else
    {   required_itteration=1;}   
}

__device__
void CalculateHadamardProduct(long int* A, long int* B, int N,long int blockDim_x,long int threadIdx_x,long int blockIdx_x) 
{
    // TODO: Write your kernel here
    long int index=blockIdx_x*blockDim_x+threadIdx_x;
    long int cols=index%N;
    long int rows=index/N;
    if(rows*N+cols<N*N)
    {   A[rows*N+cols]=A[rows*N+cols]*B[cols*N+rows];}
}

//demonstration of non perfect division
__global__ 
void CalculateHadamardProduct_boilerplate(long int* A, long int* B, int N,dim3 gridDim_original,dim3 blockDim_original)
{
    long int required_ittration,remaining_itteration;
    calc_loop_itteration(gridDim_original,blockDim_original,required_ittration,remaining_itteration);
    
    dim6_index dim_index_current;
    dim_index_current.thread_x=threadIdx.x;    
    dim_index_current.thread_y=threadIdx.y;
    dim_index_current.thread_z=threadIdx.z;
    dim_index_current.block_x=blockIdx.x;
    dim_index_current.block_y=blockIdx.y;
    dim_index_current.block_z=blockIdx.z;   
    long int id_current;
    dim6_index dim_index_original;

    for(long int a=0;a<required_ittration;a++)
    {
        id_current=get_id(dim_index_current,blockDim,gridDim,required_ittration,a);
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        
        //printf("\nblock: %ld %ld %ld thread: %ld %ld %ld",dim_index_original.block_x,dim_index_original.block_y,dim_index_original.block_z,dim_index_original.thread_x,dim_index_original.thread_y,dim_index_original.thread_z);

        CalculateHadamardProduct(A,B,N,blockDim_original.x,dim_index_original.thread_x,dim_index_original.block_x);
    }
    short id=get_id(dim_index_current,blockDim,gridDim);
    if(id<remaining_itteration)
    {
        id_current=(gridDim.x*gridDim.y*gridDim.z*blockDim.x*blockDim.y*blockDim.z*required_ittration)+id;
        //printf("\nid_current: %d remaining: %ld",id_current,remaining_itteration);
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        CalculateHadamardProduct(A,B,N,blockDim_original.x,dim_index_original.thread_x,dim_index_original.block_x);
    }
}

__device__
void FindWeightMatrix(long int* A, long int* B, int N,long int blockDim_y,long int blockDim_x,long int threadIdx_y,long int threadIdx_x,long int blockIdx_x) 
{
    // TODO: Write your kernel here
    long int thread_id=threadIdx_y*blockDim_y+threadIdx_x;
    long int index=blockIdx_x*(blockDim_x*blockDim_y)+thread_id;
    long int cols=index%N;
    long int rows=index/N;

    if((index)<(N*N))
    {
        if(A[rows*N+cols]<B[rows*N+cols])
        {   A[rows*N+cols]=B[rows*N+cols];}
    }
}

__global__
void FindWeightMatrix_boilerplate(long int* A, long int* B, int N,dim3 gridDim_original,dim3 blockDim_original)
{
    long int required_ittration,remaining_itteration;
    calc_loop_itteration(gridDim_original,blockDim_original,required_ittration,remaining_itteration);

    dim6_index dim_index_current;
    dim_index_current.thread_x=threadIdx.x;    
    dim_index_current.thread_y=threadIdx.y;
    dim_index_current.thread_z=threadIdx.z;
    dim_index_current.block_x=blockIdx.x;
    dim_index_current.block_y=blockIdx.y;
    dim_index_current.block_z=blockIdx.z;   
    long int id_current;
    dim6_index dim_index_original;

    for(long int a=0;a<required_ittration;a++)
    {
        id_current=get_id(dim_index_current,blockDim,gridDim,required_ittration,a);
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        
        FindWeightMatrix(A,B,N,blockDim_original.y,blockDim_original.x,dim_index_original.thread_y,dim_index_original.thread_x,dim_index_original.block_x);
    }
    short id=get_id(dim_index_current,blockDim,gridDim);
    if(id<remaining_itteration)
    {
        id_current=(gridDim.x*gridDim.y*gridDim.z*blockDim.x*blockDim.y*blockDim.z*required_ittration)+id;
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        
        FindWeightMatrix(A,B,N,blockDim_original.y,blockDim_original.x,dim_index_original.thread_y,dim_index_original.thread_x,dim_index_original.block_x);
    }
}

__device__
void CalculateFinalMatrix(long int* A, long int* B, int N,long int gridDim_y,long int blockDim_y,long int blockDim_x,long int threadIdx_y,long int threadIdx_x,long int blockIdx_y,long int blockIdx_x)
{
    // TODO: Write your kernel here
    long int thread_id=threadIdx_y*blockDim_y+threadIdx_x;
    long int block_id=blockIdx_y*gridDim_y+blockIdx_x;
    long int index=block_id*(blockDim_x*blockDim_y)+thread_id;
    long int cols_d=index%(N*2);
    long int rows_d=index/(N*2);
    if((index)<(2*N*2*N))
    {
        long int rows=rows_d%(N);
        long int cols=cols_d%(N);
        B[rows_d*2*N+cols_d]=B[rows_d*2*N+cols_d]*A[rows*N+cols];
    }
}

__global__
void CalculateFinalMatrix_boilerplate(long int* A, long int* B, int N,dim3 gridDim_original,dim3 blockDim_original)
{
    long int required_ittration,remaining_itteration;
    calc_loop_itteration(gridDim_original,blockDim_original,required_ittration,remaining_itteration);

    dim6_index dim_index_current;
    dim_index_current.thread_x=threadIdx.x;    
    dim_index_current.thread_y=threadIdx.y;
    dim_index_current.thread_z=threadIdx.z;
    dim_index_current.block_x=blockIdx.x;
    dim_index_current.block_y=blockIdx.y;
    dim_index_current.block_z=blockIdx.z;   
    long int id_current;
    dim6_index dim_index_original;

    for(long int a=0;a<required_ittration;a++)
    {
        id_current=get_id(dim_index_current,blockDim,gridDim,required_ittration,a);
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        
        CalculateFinalMatrix(A,B,N,gridDim_original.y,blockDim_original.y,blockDim_original.x,dim_index_original.thread_y,dim_index_original.thread_x,dim_index_original.block_y,dim_index_original.block_x);
    }
    short id=get_id(dim_index_current,blockDim,gridDim);
    if(id<remaining_itteration)
    {
        id_current=(gridDim.x*gridDim.y*gridDim.z*blockDim.x*blockDim.y*blockDim.z*required_ittration)+id;
        dim_index_original=get_dim6(id_current,blockDim_original,gridDim_original);
        
        CalculateFinalMatrix(A,B,N,gridDim_original.y,blockDim_original.y,blockDim_original.x,dim_index_original.thread_y,dim_index_original.thread_x,dim_index_original.block_y,dim_index_original.block_x);
    }
}

void print_mat(long int *mat,int size)
{
    for(int a=0;a<size;a++)
    {
        for(int b=0;b<size;b++)
        {
            printf("%d,",mat[a*size+b]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {


    int N;
    cin >> N;
    long int* A = new long int[N * N];
    long int* B = new long int[N * N];
    long int* C = new long int[N * N];
    long int* D = new long int[2 * N * 2 * N];


    for (long int i = 0; i < N * N; i++) {
        cin >> A[i];
    }

    for (long int i = 0; i < N * N; i++) {
        cin >> B[i];
    }

    for (long int i = 0; i < N * N; i++) {
        cin >> C[i];
    }

    for (long int i = 0; i < 2 * N * 2 * N; i++) {
        cin >> D[i];
    }

    /**
     * 
     * DO NOT CHANGE ANYTHING ABOVE THIS LINE
     * 
    */
    cudaSetDevice(0);
    long int* d_A;
    long int* d_B;
    long int* d_C;
    long int* d_D;

    //printf("\nA:\n");
    //print_mat(A,N);
    dim3 threadsPerBlock(1024, 1, 1);
    dim3 blocksPerGrid(ceil(N * N / 1024.0), 1, 1);
    
    cudaMalloc(&d_A,sizeof(long int)*N*N);
    cudaMalloc(&d_B,sizeof(long int)*N*N);
    cudaMemcpy(d_A,A,sizeof(long int)*N*N,cudaMemcpyHostToDevice);
    cudaMemcpy(d_B,B,sizeof(long int)*N*N,cudaMemcpyHostToDevice);
    
    auto start = std::chrono::high_resolution_clock::now();
    //CalculateHadamardProduct<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, N);//original
    dim3 threadsPerBlock_new(3,3,4);
    dim3 blocksPerGrid_new(2,1,12);
    printf("\nkernel 1");
    printf("\ntotal_threads_new: %d",blocksPerGrid_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z*threadsPerBlock_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z);
    printf("\ntotal_threads_old: %d",blocksPerGrid.x*blocksPerGrid.y*blocksPerGrid.z*threadsPerBlock.x*threadsPerBlock.y*threadsPerBlock.z);
    CalculateHadamardProduct_boilerplate<<<blocksPerGrid_new, threadsPerBlock_new>>>(d_A, d_B, N,blocksPerGrid,threadsPerBlock);
    cudaDeviceSynchronize();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed1 = end - start;
    
    cudaMemcpy(A,d_A,sizeof(long int)*N*N,cudaMemcpyDeviceToHost);
    cudaFree(d_B);
    //cudaMemcpy(B,d_B,sizeof(long int)*N*N,cudaMemcpyDeviceToHost);
    //printf("\nHarmad A:\n");
    //print_mat(A,N);
    //printf("\nC:\n");
    //print_mat(C,N);
    
    
    threadsPerBlock = dim3(32, 32, 1);
    blocksPerGrid = dim3(ceil(N * N / 1024.0), 1, 1);
    
    cudaMalloc(&d_C,sizeof(long int)*N*N);
    cudaMemcpy(d_C,C,sizeof(long int)*N*N,cudaMemcpyHostToDevice);
    
    start = std::chrono::high_resolution_clock::now();
    //FindWeightMatrix<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_C, N);//original
    threadsPerBlock_new=dim3(3,13,3);
    blocksPerGrid_new=dim3(17,6,2);
    printf("\nkernel 2");
    printf("\ntotal_threads_new: %d",blocksPerGrid_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z*threadsPerBlock_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z);
    printf("\ntotal_threads_old: %d",blocksPerGrid.x*blocksPerGrid.y*blocksPerGrid.z*threadsPerBlock.x*threadsPerBlock.y*threadsPerBlock.z);
    FindWeightMatrix_boilerplate<<<blocksPerGrid_new, threadsPerBlock_new>>>(d_A, d_C, N,blocksPerGrid, threadsPerBlock);//original
    cudaDeviceSynchronize();
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed2 = end - start;
    
    cudaMemcpy(A,d_A,sizeof(long int)*N*N,cudaMemcpyDeviceToHost);
    cudaFree(d_C);
    //printf("\nweight:\n");
    //print_mat(A,N);
    //printf("\nD:\n");
    //print_mat(D,N*2);
    

    threadsPerBlock = dim3(32, 32, 1);
    blocksPerGrid = dim3(ceil(2 * N / 32.0), ceil(2 * N / 32.0), 1);

    cudaMalloc(&d_D,sizeof(long int)*2*N*2*N);
    cudaMemcpy(d_D,D,sizeof(long int)*N*2*N*2,cudaMemcpyHostToDevice);
    
    start = std::chrono::high_resolution_clock::now();
    //CalculateFinalMatrix<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_D, N);//original
    
    threadsPerBlock_new=dim3(17,1,2);
    blocksPerGrid_new=dim3(1,3,5);
    printf("\nkernel 3");
    printf("\ntotal_threads_new: %d",blocksPerGrid_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z*threadsPerBlock_new.x*blocksPerGrid_new.y*blocksPerGrid_new.z);
    printf("\ntotal_threads_old: %d",blocksPerGrid.x*blocksPerGrid.y*blocksPerGrid.z*threadsPerBlock.x*threadsPerBlock.y*threadsPerBlock.z);
    CalculateFinalMatrix_boilerplate<<<blocksPerGrid_new, threadsPerBlock_new>>>(d_A, d_D, N, blocksPerGrid, threadsPerBlock);//original
    
    cudaDeviceSynchronize();
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed3 = end - start;

    cudaMemcpy(D,d_D,sizeof(long int)*N*2*N*2,cudaMemcpyDeviceToHost);
    cudaFree(d_A);
    // Make sure your final output from the device is stored in d_D.

    /**
     * 
     * DO NOT CHANGE ANYTHING BELOW THIS LINE
     * 
    */

    cudaMemcpy(D, d_D, 2 * N * 2 * N * sizeof(long int), cudaMemcpyDeviceToHost);

    std::ofstream file("cuda.out");
    if (file.is_open()) {
        for (long int i = 0; i < 2 * N; i++) {
            for (long int j = 0; j < 2 * N; j++) {
                file << D[i * 2 * N + j] << " ";
            }
            file << "\n";
        }
        file.close();
    } else {
        std::cout << "Unable to open file";
    }

    std::ofstream file2("cuda_timing.out");
    if(file2.is_open()) {
        file2 << elapsed1.count() << "\n";
        file2 << elapsed2.count() << "\n";
        file2 << elapsed3.count() << "\n";
        file2.close();
    } else {
        std::cout << "Unable to open file";
    }

    return 0;
}
