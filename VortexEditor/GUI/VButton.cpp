#include "VButton.h"

// Windows includes
#include <CommCtrl.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VButton::VButton() :
  VWindow(),
  m_callback(nullptr)
{
}

VButton::VButton(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VButton()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VButton::~VButton()
{
  cleanup();
}

void VButton::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_BUTTON, title.c_str(),
    WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VButton::cleanup()
{
}

void VButton::create()
{
}

void VButton::paint()
{
}

void VButton::command(WPARAM wParam, LPARAM lParam)
{
  if (!m_callback) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VButton::pressButton()
{
  MessageBox(0, "", "", 0);
}

void VButton::releaseButton()
{
}


