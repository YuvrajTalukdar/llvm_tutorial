#include<iostream>

using namespace std;

int add(int x,int y)
{
    int z;
    z=x+y;
    return z;
}

int sub(int x,int y)
{
    int z;
    z=x-y;
    return z;
}
/*
int add_sub(int x,int y)
{
    int z;
    z=add(x,y)+sub(x,y);
    return z;
}
*/
int main()
{
    cout<<add(4,7);
    cout<<sub(8,2);
    return 0;
}