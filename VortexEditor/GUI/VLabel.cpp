#include "VLabel.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VLabel::VLabel() :
  VWindow(),
  m_callback(nullptr)
{
}

VLabel::VLabel(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VLabel()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VLabel::~VLabel()
{
  cleanup();
}

void VLabel::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_STATIC, title.c_str(),
    WS_VISIBLE | WS_CHILD,
    x, y, width, height, parent.hwnd(), 0, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VLabel::cleanup()
{
}

void VLabel::create()
{
}

void VLabel::paint()
{
}

void VLabel::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (reason != CBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VLabel::pressButton()
{
}

void VLabel::releaseButton()
{
}

void VLabel::setText(std::string item)
{
  SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)item.c_str());
}

std::string VLabel::getText() const
{
  return (const char *)SendMessage(m_hwnd, WM_GETTEXT, 0, 0);
}
