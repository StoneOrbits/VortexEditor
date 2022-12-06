#pragma once

#include <Windows.h>

#include <map>
#include <string>

// The VWindow is the main window, there's only really supposed to be one
// if you want child windows that is a separate class. This is also the base
// class of all other GUI objects
class VWindow
{
public:
  // typedef for callback params
  typedef void (*VWindowCallback)(void *arg);

  VWindow();
  VWindow(HINSTANCE hinstance, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height,
    void *callbackArg);
  virtual ~VWindow();

  virtual void init(HINSTANCE hinstance, std::string title, 
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

  // add/get a child
  virtual uint32_t addChild(HMENU menuID, VWindow *child);
  virtual VWindow *getChild(HMENU menuID);

  HWND hwnd() const { return m_hwnd; }

protected:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  // window handle
  HWND m_hwnd;

  // list of children
  std::map<HMENU, VWindow *> m_children;

  // pointer to parent
  VWindow *m_pParent;

  // arg to pass to callback
  void *m_callbackArg;
};

