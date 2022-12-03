#pragma once

#include <windows.h>

class VortexEditor
{
public:
  VortexEditor();
  ~VortexEditor();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the test framework
  void run();

private:
  const COLORREF bkcolor = RGB(40, 40, 40);

  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  HINSTANCE m_hInstance;
  HBRUSH m_bkbrush;
  WNDCLASS m_wc;
  HWND m_hwnd;

};

extern VortexEditor *g_pEditor;
