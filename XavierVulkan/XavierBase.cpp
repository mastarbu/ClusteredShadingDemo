#pragma once
#include "XavierBase.h"
#include "time.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>

namespace Xavier {
  bool SDLWindow::createSDLwindow()
  {
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
      std::cout << "Could not initialize SDL." << std::endl;
      return false;
    }
    winPara.pWindow = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
    if (winPara.pWindow == NULL) 
    {
      std::cout << "Could not create SDL window." << std::endl;
      return false;
    }
    return true;
  }

  bool SDLWindow::renderLoop(XRenderBase &render)
  {
    bool stillRunning = true;
    bool sizeChanged = false;
    bool rst = true;

    while (stillRunning)
    {
      // Handle events.
      SDL_Event event;
      if (SDL_PollEvent(&event))
      {
        switch (event.type)
        {
        case SDL_QUIT:
          stillRunning = false;
          break;
         // Other cases like: window size changed.
        default:
          // Do Nothing
          continue;
        }
      }
      // Render a frame
      else
      {
        // If size of the window changed.
        if (sizeChanged)
        {
          sizeChanged = false;
          // Call the handler for window size changing.
          if (!(rst = render.onWindowSizeChanged()))
            break;
        }
        // Drawing.
        if (!(rst = render.Draw()))
          break;
      }
      // std::cout << "zavi";
      // SDL_Delay(10);
    }
    return rst;
  }

}