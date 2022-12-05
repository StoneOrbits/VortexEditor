#pragma once

#include "VWindow.h"

class VButton : public VWindow
{
public:
  // typedef for callback param
  typedef void (*VButtonCallback)(const VButton &);

  VButton();
  VButton(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t menuID, VButtonCallback callback);
  virtual ~VButton();

  virtual void init(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t menuID, VButtonCallback callback);
  virtual void cleanup();

  // windows message handlers
  virtual void create() override;
  virtual void paint() override;
  virtual void command(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton() override;
  virtual void releaseButton() override;

private:
  VButtonCallback m_callback;
};

