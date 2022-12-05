#pragma once

#include <windows.h>

#include "GUI/VWindow.h"
#include "GUI/VButton.h"

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
  // main instance
  HINSTANCE m_hInstance;

  // main window
  VWindow m_window;
  // the four buttons
  VButton m_pushButton;
  VButton m_pullButton;
  VButton m_loadButton;
  VButton m_saveButton;

  // callbacks for main actions
  static void pushCallback(const VButton &button);
  static void pullCallback(const VButton &button);
  static void loadCallback(const VButton &button);
  static void saveCallback(const VButton &button);
};

extern VortexEditor *g_pEditor;
