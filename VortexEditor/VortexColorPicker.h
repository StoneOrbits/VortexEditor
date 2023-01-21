#pragma once

// windows includes
#include <windows.h>

// gui includes
#include "GUI/VChildwindow.h"
#include "GUI/VHueSatBox.h"
#include "GUI/VValSlider.h"
#include "GUI/VComboBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

class VortexColorPicker
{
public:
  VortexColorPicker();
  ~VortexColorPicker();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the test framework
  void run();

  // show/hide the color picker window
  void show();
  void hide();

  // send the refresh message to the window
  void triggerRefresh();

private:
  // ==================================
  //  Color picker GUI

  HICON m_hIcon;

  uint32_t m_xPos;
  uint32_t m_yPos;

  // child window for color picker tool
  VChildWindow m_colorPickerWindow;

  // color ring for the color picker
  VHueSatBox m_hueSatBox;
  VValSlider m_valSlider;
};

