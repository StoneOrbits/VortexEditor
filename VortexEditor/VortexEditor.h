#pragma once

#include <windows.h>

#include "GUI/VWindow.h"

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
  HINSTANCE m_hInstance;
  VWindow m_window;
};

extern VortexEditor *g_pEditor;
