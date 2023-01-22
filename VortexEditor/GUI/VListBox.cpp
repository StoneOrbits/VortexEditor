#include "VListBox.h"

// Windows includes
#include <CommCtrl.h>
#include <windowsx.h>

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
  uintptr_t menuID, VWindowCallback callback) :
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
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_LISTBOX, title.c_str(),
    WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
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
  int reason = HIWORD(wParam);
  if (!m_callback || reason != LBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VListBox::pressButton(WPARAM wParam, LPARAM lParam)
{
}

void VListBox::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

void VListBox::addItem(string item)
{
  ListBox_AddString(m_hwnd, item.c_str());
}

int VListBox::getSelection() const
{
  return ListBox_GetCurSel(m_hwnd);
}

void VListBox::setSelection(int selection)
{
  ListBox_SetCurSel(m_hwnd, selection);
}

void VListBox::clearItems()
{
  SendMessage(m_hwnd, LB_RESETCONTENT, 0, 0);
}
