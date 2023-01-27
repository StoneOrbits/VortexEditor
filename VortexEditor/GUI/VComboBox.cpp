#include "VComboBox.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

VComboBox::VComboBox() :
  VWindow(),
  m_callback(nullptr)
{
}

VComboBox::VComboBox(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VComboBox()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VComboBox::~VComboBox()
{
  cleanup();
}

void VComboBox::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  if (!menuID) {
    menuID = nextMenuID++;
  }

  if (!parent.addChild(menuID, this)) {
    return;
  }

  // create the window
  m_hwnd = CreateWindow(WC_COMBOBOX, title.c_str(),
    CBS_SIMPLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VSCROLL | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VComboBox::cleanup()
{
}

void VComboBox::create()
{
}

void VComboBox::paint()
{
}

void VComboBox::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (!m_callback || reason != CBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg, this);
}

void VComboBox::pressButton(WPARAM wParam, LPARAM lParam)
{
}

void VComboBox::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

void VComboBox::addItem(std::string item)
{
  ComboBox_AddString(m_hwnd, item.c_str());
  if (getSelection() == -1) {
    setSelection(0);
  }
}

int VComboBox::getSelection() const
{
  return ComboBox_GetCurSel(m_hwnd);
}

void VComboBox::setSelection(int selection)
{
  ComboBox_SetCurSel(m_hwnd, selection);
}

void VComboBox::clearItems()
{
  SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);
}

string VComboBox::getSelectionText() const
{
  char str[256] = {0};
  ComboBox_GetText(m_hwnd, str, sizeof(str));
  return str;
}
