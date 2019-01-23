#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace Xavier {
  class XRenderBase {
  // Methods
  public:
    virtual bool Draw() = 0;
    virtual bool onWindowSizeChanged() = 0;
    virtual void CleanUp() = 0;

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