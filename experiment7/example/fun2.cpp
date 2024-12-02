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

int add_sub(int x,int y)
{
    int z;
    z=add(x,y)+sub(x,y);
    return z;
}

int main()
{
    int x=7;
    for(int a=0;a<20;a++)//E
    {
        x+=add(4,a);
        x=x*4+sub(a,x);
    }
    cout<<x;
    return 0;
}