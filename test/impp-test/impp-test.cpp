#include <iostream>
#include <tga.hpp>
#include <bmp.hpp>

int main()
{
    using namespace impp;

    error::set_error_handler([](const auto& err){ std::cout << err.what() << std::endl; });
    
    // TESTING TGA IMAGES
    auto test = tga::load<pixel32rgba>("init.tga");
    if(test.empty())
        std::cout << "loading tga failed!" << std::endl;

    else
    {
        tga::save_to_file<tga::tga_type::TGA_RLE_RBG>(test, "final_rle.tga");
        tga::save_to_file<tga::tga_type::TGA_UNCOMPRESSED_MAPPED>(test, "final_umap.tga");
        tga::save_to_file<tga::tga_type::TGA_UNCOMPRESSED_RGB>(test, "final_urgb.tga");
    }

    return 0;
}
