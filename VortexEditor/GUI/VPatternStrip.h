#pragma once

#include "VWindow.h"

#include "VLabel.h"

#include "VortexLib.h"

#include <deque>

class VPatternStrip : public VWindow
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
  typedef void (*VPatternStripCallback)(void *arg, VPatternStrip *select, SelectEvent sevent);

  VPatternStrip();
  VPatternStrip(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t linewidth, const json &modeData, uintptr_t menuID, VPatternStripCallback callback);
  virtual ~VPatternStrip();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t linewidth, const json &modeData, uintptr_t menuID, VPatternStripCallback callback);
  virtual void cleanup();

  virtual void run();

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
  uint32_t getColor() const;
  std::string getColorName() const;
  void setColor(std::string name);

  bool isActive() const;
  void setActive(bool active);
  bool isSelected() const;
  void setSelected(bool selected);

  void setLabelEnabled(bool enabled);
  void setSelectable(bool selectable);

private:
  static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
  static WNDCLASS m_wc;

  // local vortex instance to render community modes
  Vortex m_vortex;

  // internal vgui label for display of color code
  VLabel m_colorLabel;

  VPatternStripCallback m_callback;
  // the color in this slot
  uint32_t m_color;
  // whether this slot is actually used
  bool m_active;
  // whether this slot is selected for changes
  bool m_selected;
  // whether this control is selectable
  bool m_selectable;

  // vector of colors
  std::deque<RGBColor> m_colorSequence;
  // the line width
  uint32_t m_lineWidth;
  // number of 'slices' (slices = width / line size)
  uint32_t m_numSlices;
};

