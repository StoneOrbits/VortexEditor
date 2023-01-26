#pragma once

#include "VWindow.h"

class VTextBox : public VWindow
{
public:
  VTextBox();
  VTextBox(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual ~VTextBox();

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
  void setText(std::string item, bool notify = true);
  void clearText();
  std::string getText() const;
  uint8_t getValue() const;

  void enableChangeNotifications(bool enabled);

private:
  VWindowCallback m_callback;
  bool m_notifications;
};
