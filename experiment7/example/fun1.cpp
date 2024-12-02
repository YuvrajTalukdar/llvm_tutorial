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
    int x=7;
    x+=add(4,7);
    cout<<x;
    cout<<sub(8,2);
    return 0;
}