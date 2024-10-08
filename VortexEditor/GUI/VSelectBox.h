#pragma once

#include "VWindow.h"

#include "VLabel.h"

class RGBColor;

class VSelectBox : public VWindow
{
public:
  // type of selection event for callback
  enum SelectEvent {
    SELECT_PRESS,
    SELECT_CTRL_PRESS,
    SELECT_SHIFT_PRESS,
    SELECT_DRAG,
    SELECT_RELEASE
  };
  // callback upon selection
  typedef void (*VSelectBoxCallback)(void *arg, uint32_t x, uint32_t y, SelectEvent sevent);

  VSelectBox();
  VSelectBox(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VSelectBoxCallback callback);
  virtual ~VSelectBox();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VSelectBoxCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
  virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;
  virtual void mouseMove(uint32_t buttons, uint16_t x, uint16_t y);

  // set a background via callback to convert x/y to rgb
  //void setBackground(VFillCallback fillCallback);
  void setBackground(HBITMAP hBitmap);
  void setHoverBackground(HBITMAP hBitmap);
  void setBackgroundTransparency(bool transparent);
  void setBorderSize(uint32_t borderSize);
  void setSelection(uint32_t x, uint32_t y);

  // whether to draw specific components
  void setDrawHLine(bool draw) { m_drawHLine = draw; }
  void setDrawVLine(bool draw) { m_drawVLine = draw; }
  void setDrawCircle(bool draw) { m_drawCircle = draw; }

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  void doCallback(SelectEvent sevent);

  uint32_t m_borderSize;

  uint32_t m_width;
  uint32_t m_height;

  // local coords of the inner rectangle that contains the actual
  uint32_t m_innerLeft;
  uint32_t m_innerTop;
  uint32_t m_innerRight;
  uint32_t m_innerBottom;
  
  uint32_t m_innerWidth;
  uint32_t m_innerHeight;

  bool m_drawHLine;
  bool m_drawVLine;
  bool m_drawCircle;

  bool m_pressed;
  bool m_useTransparency;
  
  bool m_mouseInside;

  uint32_t m_xSelect;
  uint32_t m_ySelect;

  // internal vgui label for display of color code
  VLabel m_colorLabel;

  VSelectBoxCallback m_callback;
  HBITMAP m_bitmap;
  HBITMAP m_hoverBitmap;
};
