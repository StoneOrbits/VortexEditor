#include "VTextBox.h"

// Windows includes
#include <CommCtrl.h>
#include <windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VTextBox::VTextBox() :
  VWindow(),
  m_callback(nullptr)
{
}

VTextBox::VTextBox(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VTextBox()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VTextBox::~VTextBox()
{
  cleanup();
}

void VTextBox::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_EDIT, title.c_str(),
    WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VTextBox::cleanup()
{
}

void VTextBox::create()
{
}

void VTextBox::paint()
{
}

void VTextBox::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (!m_callback || reason != EN_CHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VTextBox::pressButton(WPARAM wParam, LPARAM lParam)
{
}

void VTextBox::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

void VTextBox::setText(std::string item)
{
  Edit_SetText(m_hwnd, item.c_str());
}

void VTextBox::clearText()
{
  setText("");
}

string VTextBox::getText() const
{
  char text[256] = {0};
  Edit_GetText(m_hwnd, text, sizeof(text));
  return text;
}

uint8_t VTextBox::getValue() const
{
  uint32_t val = strtoul(getText().c_str(), NULL, 10);
  if (val > UINT8_MAX) {
    val = UINT8_MAX;
  }
  return (uint8_t)val;
}