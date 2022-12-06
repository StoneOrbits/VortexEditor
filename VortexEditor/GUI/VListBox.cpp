#include "VListBox.h"

// Windows includes
#include <CommCtrl.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VListBox::VListBox() :
  VWindow(),
  m_callback(nullptr)
{
}

VListBox::VListBox(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t menuID, VWindowCallback callback) :
  VListBox()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VListBox::~VListBox()
{
  cleanup();
}

void VListBox::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  parent.addChild((HMENU)menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_LISTBOX, title.c_str(),
    WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  ShowWindow(m_hwnd, SW_NORMAL);
}

void VListBox::cleanup()
{
}

void VListBox::create()
{
}

void VListBox::paint()
{
}

void VListBox::command(WPARAM wParam, LPARAM lParam)
{
  m_callback(m_callbackArg);
}

void VListBox::pressButton()
{
  MessageBox(0, "", "", 0);
}

void VListBox::releaseButton()
{
}


