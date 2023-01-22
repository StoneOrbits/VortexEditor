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

  // typedef for background filling callback
  typedef COLORREF (*VFillCallback)(uint32_t x, uint32_t y);

  // callbacks for selecting sv and h
  static void selectSVCallback(void *pthis, uint32_t x, uint32_t y) { ((VortexColorPicker *)pthis)->selectSV(x, y); }
  static void selectHCallback(void *pthis, uint32_t x, uint32_t y) { ((VortexColorPicker *)pthis)->selectH(y); }

  void genSVBackgrounds();
  HBITMAP genSVBackground(uint32_t hue);
  HBITMAP genHueBackground(uint32_t width, uint32_t height);

  void selectSV(uint32_t sat, uint32_t val);
  void selectH(uint32_t hue);

  // array of bitmaps for the SV selector background
  HBITMAP m_svBitmaps[255];

  // bitmap for the H selector background
  HBITMAP m_hueBitmap;

  HICON m_hIcon;

  uint32_t m_xPos;
  uint32_t m_yPos;

  HSVColor m_curColor;

  // child window for color picker tool
  VChildWindow m_colorPickerWindow;

  // huesat box and value slider
  VSelectBox m_satValBox;
  VSelectBox m_hueSlider;

  // preview of color
  VColorSelect m_colorPreview;

  // hue/sat/val text entry
  VTextBox m_hueTextbox;
  VTextBox m_satTextbox;
  VTextBox m_valTextbox;
};

