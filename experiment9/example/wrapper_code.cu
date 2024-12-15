#include<iostream>

__global__
void foo123()
{
    unsigned long int id_current=threadIdx.x;
    unsigned long int max_id_original;
    for(unsigned long int a=0;a<id_current;a++)//TLoop
    {
        
        if(id_current < max_id_original)
        {
            printf("test");
            //User's kernel (now a device function will be placed here);

        }
    }
}

__global__
void foo()
{
    unsigned long int id_current=threadIdx.x;
    unsigned long int max_id_original;
    for(unsigned long int a=0;a<id_current;a++)//TLoop
    {
        
        if(id_current < max_id_original)
        {
            printf("test1234");
            //User's kernel (now a device function will be placed here);

        }
    }
}