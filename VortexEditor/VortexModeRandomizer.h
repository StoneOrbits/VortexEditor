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

class VortexModeRandomizer
{
public:
  VortexModeRandomizer();
  ~VortexModeRandomizer();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the mode randomizer
  void run();

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();

  bool isOpen() const { return m_isOpen; }
  
  HWND hwnd() const { return m_modeRandomizerWindow.hwnd(); }

private:
  // ==================================
  //  Mode Randomizer GUI
  static void hideGUICallback(void *pthis, VWindow *window) {
    ((VortexModeRandomizer *)pthis)->hide();
  }
  static void loseFocusCallback(void *pthis, VWindow *window) {
    ((VortexModeRandomizer *)pthis)->loseFocus();
  }

  bool m_isOpen;

  HICON m_hIcon;

  // child window for mode randomizer tool
  VChildWindow m_modeRandomizerWindow;

};

