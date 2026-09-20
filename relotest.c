#include <windows.h>

int big1[50000];
int big2[50000];
int big3[50000];
int big4[50000];

int main()
{
    big1[0] = 1;
    big2[0] = 2;
    big3[0] = 3;
    big4[0] = 4;
    return big1[0] + big2[0] + big3[0] + big4[0];
}
