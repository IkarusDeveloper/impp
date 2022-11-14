#include <iostream>
#include <tga.hpp>
#include <bmp.hpp>

int main()
{
    using namespace impp;
    
    auto image = bmp::load<pixel32rgba>("init.bmp");

    return 0;
}
