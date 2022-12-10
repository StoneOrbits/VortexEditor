#include "VColorSelect.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"
#include "Colors/ColorTypes.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

#define WC_COLOR_SELECT "VColorSelect"

WNDCLASS VColorSelect::m_wc = {0};

VColorSelect::VColorSelect() :
  VWindow(),
  m_callback(nullptr),
  m_color(0)
{
}

VColorSelect::VColorSelect(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t menuID, VWindowCallback callback) :
  VColorSelect()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VColorSelect::~VColorSelect()
{
  cleanup();
}

void VColorSelect::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  parent.addChild((HMENU)menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_COLOR_SELECT, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VColorSelect::cleanup()
{
}

void VColorSelect::create()
{
}

static HBRUSH getBrushCol(DWORD rgbcol)
{
  static std::map<COLORREF, HBRUSH> m_brushmap;
  HBRUSH br;
  COLORREF col = RGB(rgbcol & 0xFF, (rgbcol >> 8) & 0xFF, (rgbcol >> 16) & 0xFF);
  if (m_brushmap.find(col) == m_brushmap.end()) {
    br = CreateSolidBrush(col);
    m_brushmap[col] = br;
  }
  br = m_brushmap[col];
  return br;
}

void VColorSelect::paint()
{
  PAINTSTRUCT paintStruct;
  memset(&paintStruct, 0, sizeof(paintStruct));
  HDC hdc = BeginPaint(m_hwnd, &paintStruct);
  RECT rect;
  GetClientRect(m_hwnd, &rect);
  FillRect(hdc, &rect, getBrushCol(0));
  rect.left += 2;
  rect.top += 2;
  rect.right -= 2;
  rect.bottom -= 2;
  FillRect(hdc, &rect, getBrushCol((COLORREF)m_color));
  EndPaint(m_hwnd, &paintStruct);
}

void VColorSelect::command(WPARAM wParam, LPARAM lParam)
{
  int reason = HIWORD(wParam);
  if (reason != CBN_SELCHANGE) {
    return;
  }
  m_callback(m_callbackArg);
}

void VColorSelect::pressButton()
{
  //MessageBox(0, "", "", 0);
}

void VColorSelect::releaseButton()
{
}

void VColorSelect::addItem(std::string item)
{
  //ColorSelect_AddString(m_hwnd, item.c_str());
  if (getSelection() == -1) {
    setSelection(0);
  }
}

int VColorSelect::getSelection() const
{
  return 0; //ColorSelect_GetCurSel(m_hwnd);
}

void VColorSelect::setSelection(int selection)
{
  //ColorSelect_SetCurSel(m_hwnd, selection);
}

void VColorSelect::clearItems()
{
  SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);
}

void VColorSelect::setColor(uint32_t col)
{
  m_color = col;
  RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

LRESULT CALLBACK VColorSelect::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VColorSelect *pColorSelect = (VColorSelect *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!pColorSelect) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  switch (uMsg) {
  case WM_VSCROLL:
    break;
  case WM_LBUTTONDOWN:
    pColorSelect->pressButton();
    break;
  case WM_LBUTTONUP:
    pColorSelect->releaseButton();
    break;
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)pColorSelect->m_wc.hbrBackground;
  case WM_CREATE:
    pColorSelect->create();
    break;
  case WM_PAINT:
    pColorSelect->paint();
    return 0;
  //case WM_LBUTTONDOWN:
    //g_pEditor->handleWindowClick(LOWORD(lParam), HIWORD(lParam));
    //break;
  case WM_COMMAND:
    pColorSelect->command(wParam, lParam);
    break;
  case WM_DESTROY:
    pColorSelect->cleanup();
    // TODO: proper cleanup
    PostQuitMessage(0);
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void VColorSelect::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VColorSelect::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VWindow::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.lpszClassName = EDITOR_CLASS;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_COLOR_SELECT;
  RegisterClass(&m_wc);
}
