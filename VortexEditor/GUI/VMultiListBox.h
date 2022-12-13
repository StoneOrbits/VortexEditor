#pragma once

#include "VWindow.h"

#include <vector>

class VMultiListBox : public VWindow
{
public:
  VMultiListBox();
  VMultiListBox(HINSTANCE hinstance, VWindow &parent, const std::string &title, 
    COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
    uintptr_t menuID, VWindowCallback callback);
  virtual ~VMultiListBox();

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

  // item control
  void addItem(std::string item);
  void getSelections(std::vector<int> &selections) const;
  int getSelection() const;
  int numItems() const;
  int numSelections() const;
  void setSelections(const std::vector<int> &selections);
  void setSelection(int sel);
  void clearSelection(int sel);
  void clearSelections();
  void clearItems();

private:
  VWindowCallback m_callback;
};
