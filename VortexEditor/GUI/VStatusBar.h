#pragma once

#include "VWindow.h"

class VStatusBar : public VWindow
{
public:
  VStatusBar();
  VStatusBar(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual ~VStatusBar();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
  virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;

  // item control
  void setText(std::string item);
  void clearText();
  std::string getText() const;

  // set status
  void setStatus(COLORREF col, std::string status);

private:
  VWindowCallback m_callback;
};
