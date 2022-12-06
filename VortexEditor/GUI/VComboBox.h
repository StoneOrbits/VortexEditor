#pragma once

#include "VWindow.h"

class VComboBox : public VWindow
{
public:
  VComboBox();
  VComboBox(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uint32_t menuID, VWindowCallback callback);
  virtual ~VComboBox();

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

private:
  VWindowCallback m_callback;
};
