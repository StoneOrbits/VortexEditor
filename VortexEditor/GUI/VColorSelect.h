#pragma once

#include "VWindow.h"

class VColorSelect : public VWindow
{
public:
  VColorSelect();
  VColorSelect(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t menuID, VWindowCallback callback);
  virtual ~VColorSelect();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t menuID, VWindowCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton() override;
  virtual void releaseButton() override;

  void addItem(std::string item);
  int getSelection() const;
  void setSelection(int selection);
  void clearItems();

  void setColor(uint32_t col);

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  VWindowCallback m_callback;
  uint32_t m_color;
};
