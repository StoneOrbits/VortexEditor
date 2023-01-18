#pragma once

#include "VWindow.h"

// The window class
#define WC_VCHILDWINDOW      "VCHILDWINDOW"

// The VChildWindow is the main window, there's only really supposed to be one
// if you want child windows that is a separate class. This is also the base
// class of all other GUI objects
class VChildWindow : public VWindow
{
public:
  VChildWindow();
  VChildWindow(HINSTANCE hinstance, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height,
    void *callbackArg);
  virtual ~VChildWindow();

  virtual void init(HINSTANCE hinstance, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height,
    void *callbackArg);
  virtual void cleanup();

  virtual bool process(MSG &msg);

  // windows message handlers
  virtual void create();
  virtual void paint();
  virtual void command(WPARAM wParam, LPARAM lParam);
  virtual void pressButton();
  virtual void releaseButton();

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;
};
