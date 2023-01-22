#include "VStatusBar.h"

// Windows includes
#include <CommCtrl.h>
#include <windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VStatusBar::VStatusBar() :
  VWindow(),
  m_callback(nullptr)
{
}

VStatusBar::VStatusBar(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VStatusBar()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VStatusBar::~VStatusBar()
{
  cleanup();
}

void VStatusBar::init(HINSTANCE hInstance, VWindow &parent, const string &title,
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
    WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_READONLY,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VStatusBar::cleanup()
{
}

void VStatusBar::create()
{
}

void VStatusBar::paint()
{
}

void VStatusBar::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (!m_callback || reason != EN_CHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VStatusBar::pressButton(WPARAM wParam, LPARAM lParam)
{
}

void VStatusBar::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

void VStatusBar::setText(std::string item)
{
  Edit_SetText(m_hwnd, item.c_str());
}

void VStatusBar::clearText()
{
  setText("");
}

string VStatusBar::getText() const
{
  char text[256] = {0};
  Edit_GetText(m_hwnd, text, sizeof(text));
  return text;
}

void VStatusBar::setStatus(COLORREF col, std::string status)
{
  setForeColor(col);
  setText(" " + status);
}
