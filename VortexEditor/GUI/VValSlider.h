#pragma once

#include "VWindow.h"

#include "VLabel.h"

class RGBColor;

class VValSlider : public VWindow
{
public:
  VValSlider();
  VValSlider(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual ~VValSlider();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton() override;
  virtual void releaseButton() override;

  // window message for right button press, only exists here
  void rightButtonPress();

  void clear();
  void setColor(uint32_t col);
  void setFlippedColor(uint32_t col);
  uint32_t getColor() const;
  std::string getColorName() const;
  void setColor(std::string name);
  uint32_t getFlippedColor() const;

  bool isActive() const;
  void setActive(bool active);

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  uint32_t m_borderSize;
  uint32_t m_width;
  uint32_t m_height;

  uint32_t m_valPos;

  // internal vgui label for display of color code
  VLabel m_colorLabel;

  VWindowCallback m_callback;
  HBITMAP m_bitmap;
};


