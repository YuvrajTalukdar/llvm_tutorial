#include<iostream>

using namespace std;

int foo1(int x)
{   
    return x+1;
}

int foo2(int a,int b)
{
    int result=0;
    for(int a=0;a<b;a++)
    {
        result=a+b*a;
    }
    return result;
}
int main()
{
    int a=4;
    a=foo1(a);    
    a=foo2(a,4);
    cout<<a;
    return a;
}
