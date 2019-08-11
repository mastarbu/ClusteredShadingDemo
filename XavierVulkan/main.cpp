#define SDL_MAIN_HANDLED

#include <iostream>
#include "XavierBase.h"
// #include "XSampleAlpha.h"
// #include "XSampleTeapot.h"
#include "XSampleSponza.h"

int main()
{
    Xavier::SDLWindow window;
    if (!window.createSDLwindow())
    {
        std::cout << "Rendering Failed: No Available Window." << std::endl;
        return -1;
    }

    Xavier::XSampleSponza sampleAlpha;
    if (sampleAlpha.Prepare(window))
        window.renderLoop(sampleAlpha);
    else
        std::cout << "Preparation for the vulkan failed." << std::endl;
    return 0;
}