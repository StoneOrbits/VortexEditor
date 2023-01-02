#include "VSlider.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VSlider::VSlider() :
  VWindow(),
  m_callback(nullptr)
{
}

VSlider::VSlider(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VSlider()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VSlider::~VSlider()
{
  cleanup();
}

void VSlider::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(TRACKBAR_CLASS, title.c_str(),
    WS_VISIBLE | WS_CHILD | WS_TABSTOP | TBS_VERT,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VSlider::cleanup()
{
}

void VSlider::create()
{
}

void VSlider::paint()
{
}

void VSlider::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (!m_callback || reason != CBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VSlider::pressButton()
{
}

void VSlider::releaseButton()
{
}
