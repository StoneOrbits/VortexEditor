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
  void refreshColor();

  bool isOpen() const { return m_isOpen; }
  
  void setColor(HSVColor col) { m_curHSV = col; m_curRGB = m_curHSV; }
  void setColor(RGBColor col) { m_curRGB = col; m_curHSV = m_curRGB; }
  HSVColor getRGB() const { return m_curRGB; }
  RGBColor getHSV() const { return m_curHSV; }

private:
  // loader thread
  static DWORD __stdcall loadThread(void *arg);
  // trigger background loader
  void load();
  // whether background loader is done
  bool loaded();

  // ==================================
  //  Color picker GUI

  // typedef for background filling callback
  typedef COLORREF (*VFillCallback)(uint32_t x, uint32_t y);

  // callbacks for selecting sv and h
  static void selectSVCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent) { ((VortexColorPicker *)pthis)->selectSV(sevent, x, y); }
  static void selectHCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent)  { ((VortexColorPicker *)pthis)->selectH(sevent, y); }
  static void selectRCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent)  { ((VortexColorPicker *)pthis)->selectR(sevent, y); }
  static void selectGCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent)  { ((VortexColorPicker *)pthis)->selectG(sevent, y); }
  static void selectBCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent)  { ((VortexColorPicker *)pthis)->selectB(sevent, y); }
  static void fieldEditCallback(void *pthis, VWindow *window)       { ((VortexColorPicker *)pthis)->fieldEdit(window); }
  static void hideGUICallback(void *pthis, VWindow *window)         { ((VortexColorPicker *)pthis)->hide(); }
  static void historyCallback(void *pthis, VColorSelect *colSelect, VColorSelect::SelectEvent sevent)       { 
    if (sevent == VColorSelect::SelectEvent::SELECT_LEFT_CLICK) {
      ((VortexColorPicker *)pthis)->recallHistory(colSelect);
    }
  }

  void selectSV(VSelectBox::SelectEvent sevent, uint32_t s, uint32_t v);
  void selectH(VSelectBox::SelectEvent sevent, uint32_t h);
  void fieldEdit(VWindow *window);

  void selectR(VSelectBox::SelectEvent sevent, uint32_t r);
  void selectG(VSelectBox::SelectEvent sevent, uint32_t g);
  void selectB(VSelectBox::SelectEvent sevent, uint32_t b);

  void pickCol(const RGBColor &col);

  void genSVBackgrounds();
  HBITMAP genSVBackground(uint32_t hue);
  HBITMAP genHueBackground(uint32_t width, uint32_t height);
  HBITMAP genRGBBackground(uint32_t width, uint32_t height, int rmult, int gmult, int bmult);
  void pushHistory(uint32_t rawCol);
  void recallHistory(VColorSelect *history);

  void selectS(uint32_t sat);
  void selectV(uint32_t val);
  void selectSV(uint32_t sat, uint32_t val);
  void selectH(uint32_t hue);

  void selectR(uint32_t r);
  void selectG(uint32_t g);
  void selectB(uint32_t b);

  bool m_isOpen;

  // mutex that is posted once we're loaded
  HANDLE m_mutex;
  HANDLE m_loadThread;

  // array of bitmaps for the SV selector background
  HBITMAP m_svBitmaps[256];

  // bitmap for the H selector background
  HBITMAP m_hueBitmap;

  // rgb slider bitmaps
  HBITMAP m_redBitmap;
  HBITMAP m_greenBitmap;
  HBITMAP m_blueBitmap;

  HICON m_hIcon;

  uint32_t m_xPos;
  uint32_t m_yPos;

  uint64_t m_lastRefresh;
  uint32_t m_lastCol;

  RGBColor m_curRGB;
  HSVColor m_curHSV;

  // child window for color picker tool
  VChildWindow m_colorPickerWindow;

  // huesat box and value slider
  VSelectBox m_satValBox;
  VSelectBox m_hueSlider;

  VSelectBox m_redSlider;
  VSelectBox m_greenSlider;
  VSelectBox m_blueSlider;

  // preview of color
  VColorSelect m_colorPreview;

  // color histories
  VColorSelect m_colorHistory[5];

  // color histories
  VColorSelect m_savedColorsBox;
  VColorSelect m_savedColors[8];

  //  text entry
  VTextBox m_hueTextbox;
  VTextBox m_satTextbox;
  VTextBox m_valTextbox;

  // text entry labels
  VLabel m_hueLabel;
  VLabel m_satLabel;
  VLabel m_valLabel;

  // rgb text entry
  VTextBox m_redTextbox;
  VTextBox m_grnTextbox;
  VTextBox m_bluTextbox;

  // text entry labels
  VLabel m_redLabel;
  VLabel m_grnLabel;
  VLabel m_bluLabel;

  // hex entry
  VTextBox m_hexTextbox;

  // hex label
  VLabel m_hexLabel;
};

