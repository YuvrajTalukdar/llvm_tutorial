#include<stdio.h>

int callee(const int *x)
{   return *x+1;}

int main()
{
    int T=4;
    return callee(&T);    
}