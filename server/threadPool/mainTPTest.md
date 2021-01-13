#include "taskController.h"
#include <unistd.h>

using namespace WG;

int main()
{
    taskController tc;
    int i =0;
    while(1)
    {
        tc.makeNewTask();
        usleep(50000);
        //++i;
    }
}