#pragma once

#include "VWindow.h"

#include "VLabel.h"

class VColorSelect : public VWindow
{
public:
  // type of selection event for callback
  enum SelectEvent {
    SELECT_LEFT_CLICK,
    SELECT_CTRL_LEFT_CLICK,
    SELECT_SHIFT_LEFT_CLICK,
    SELECT_RIGHT_CLICK,
  };
  // callback upon selection
  typedef void (*VColorSelectCallback)(void *arg, VColorSelect *select, SelectEvent sevent);

  VColorSelect();
  VColorSelect(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VColorSelectCallback callback);
  virtual ~VColorSelect();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VColorSelectCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
  virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;

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
  bool isSelected() const;
  void setSelected(bool selected);

  void setLabelEnabled(bool enabled);

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  // internal vgui label for display of color code
  VLabel m_colorLabel;

  VColorSelectCallback m_callback;
  // the color in this slot
  uint32_t m_color;
  // whether this slot is actually used
  bool m_active;
  // whether this slot is selected for changes
  bool m_selected;
};
