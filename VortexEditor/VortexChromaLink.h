#pragma once

// windows includes
#include <windows.h>

// gui includes
#include "GUI/VChildwindow.h"
#include "GUI/VColorSelect.h"
#include "GUI/VSelectBox.h"
#include "GUI/VComboBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

#include "Colors/Colortypes.h"

class VortexChromaLink
{
public:
  VortexChromaLink();
  ~VortexChromaLink();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the mode randomizer
  void run();

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();
  void pullDuoMode();
  void pushDuoMode();

  bool isOpen() const { return m_isOpen; }
  
  HWND hwnd() const { return m_chromaLinkWindow.hwnd(); }

private:
  // ==================================
  //  Mode Randomizer GUI
  static void hideGUICallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->hide();
  }
  static void loseFocusCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->loseFocus();
  }
  static void pullCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pullDuoMode();
  }
  static void pushCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pushDuoMode();
  }

  bool m_isOpen;

  HICON m_hIcon;

  // child window for mode randomizer tool
  VChildWindow m_chromaLinkWindow;

  // pull from the connected duo via chromadeck
  VButton m_pullButton;
  VButton m_pushButton;

};
