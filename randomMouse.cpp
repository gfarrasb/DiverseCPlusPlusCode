
#include <stdlib.h>
#include <time.h>
#include<iostream>
#include<windows.h>
using namespace std;
 
int main()
{
    int numX,numY, c;
    srand(time(NULL));
    
    while(1)
    {
        numX = 1 + rand() % (1001 - 1);
        numY = 1 + rand() % (1001 - 1);
        SetCursorPos(numX,numY);
        Sleep (200);
        
    }
    
    return 0;
}
