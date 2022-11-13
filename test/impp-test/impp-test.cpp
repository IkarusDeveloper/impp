#include <iostream>
#include <tga.hpp>

int main()
{
    using namespace impp;
    auto img = tga::load<pixel32rgba>("init.tga");
    auto pixels = pixel_convert<pixel24rgb>(img.pixels);
    auto conv = image<pixel24rgb>::create(img.width, img.height, std::move(pixels));
    tga::save(conv, "final.tga");
    return 0;
}
