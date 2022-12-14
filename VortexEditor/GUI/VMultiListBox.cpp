#include "VMultiListBox.h"

// Windows includes
#include <CommCtrl.h>
#include <windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VMultiListBox::VMultiListBox() :
  VWindow(),
  m_callback(nullptr)
{
}

VMultiListBox::VMultiListBox(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VMultiListBox()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VMultiListBox::~VMultiListBox()
{
  cleanup();
}

void VMultiListBox::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_LISTBOX, title.c_str(),
    WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP | LBS_EXTENDEDSEL,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VMultiListBox::cleanup()
{
}

void VMultiListBox::create()
{
}

void VMultiListBox::paint()
{
}

void VMultiListBox::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (reason != LBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VMultiListBox::pressButton()
{
}

void VMultiListBox::releaseButton()
{
}

void VMultiListBox::addItem(string item)
{
  ListBox_AddString(m_hwnd, item.c_str());
}

void VMultiListBox::getSelections(vector<int> &selections) const
{
  int numSel = ListBox_GetSelCount(m_hwnd);
  if (numSel <= 0) {
    return;
  }
  int *sel = new int[numSel];
  if (!sel) {
    return;
  }
  ListBox_GetSelItems(m_hwnd, numSel, sel);
  selections.insert(selections.end(), sel, sel + numSel);
  delete[] sel;
}

int VMultiListBox::getSelection() const
{
  return ListBox_GetCurSel(m_hwnd);
}

void VMultiListBox::setSelections(const std::vector<int> &selections)
{
  // don't think we need this atm
  for (uint32_t i = 0; i < selections.size(); ++i) {
    setSelection(selections[i]);
  }
}

int VMultiListBox::numItems() const
{
  return ListBox_GetCount(m_hwnd);
}

int VMultiListBox::numSelections() const
{
  return ListBox_GetSelCount(m_hwnd);
}

void VMultiListBox::setSelection(int selection)
{
  ListBox_SetSel(m_hwnd, true, selection);
}

void VMultiListBox::clearSelection(int selection)
{
  ListBox_SetSel(m_hwnd, false, selection);
}

void VMultiListBox::clearSelections()
{
  int numSel = numItems();
  for (uint32_t i = 0; i < numSel; ++i) {
    clearSelection(i);
  }
}

void VMultiListBox::clearItems()
{
  SendMessage(m_hwnd, LB_RESETCONTENT, 0, 0);
}