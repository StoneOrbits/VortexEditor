#pragma once

#include "VWindow.h"

class VColorSelect : public VWindow
{
public:
  VColorSelect();
  VColorSelect(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual ~VColorSelect();

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
  uint32_t getColor() const;

  bool isActive() const;
  void setActive(bool active);

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  VWindowCallback m_callback;
  // the color in this slot
  uint32_t m_color;
  // whether this slot is actually used
  bool m_active;
};
