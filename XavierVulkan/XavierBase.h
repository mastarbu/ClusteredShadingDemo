#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace Xavier {
  class SDLWindow;
  class XRenderBase {
  // Methods
  public:
    virtual bool Prepare(SDLWindow &window) = 0;
    virtual bool Draw() = 0;
    virtual bool onWindowSizeChanged() = 0;

    bool readyToDraw() { return CanRender; }

  // Attributes
  private:
    bool CanRender = false;
  };
  
  struct SDLWindowPara{
   SDL_Window *pWindow;
  };


  class SDLWindow {
  public:
    bool createSDLwindow();
    bool renderLoop(XRenderBase &renderer);

    SDLWindowPara getSDLWindowParameters() { return winPara; }

  private:
    SDLWindowPara winPara;
  };
}