#include <iostream>
#include "uthreads.h"

int main()
{
    std::cout << "Hello, World!" << std::endl;
    uthread_init(8);
    return 0;
}
