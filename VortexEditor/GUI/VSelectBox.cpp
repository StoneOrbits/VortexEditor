#include "VSelectBox.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"
#include "Colors/ColorTypes.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

#define WC_HUE_SAT_BOX "VSelectBox"

#define PI 3.141592654

WNDCLASS VSelectBox::m_wc = {0};

VSelectBox::VSelectBox() :
  VWindow(),
  m_borderSize(1),
  m_width(0),
  m_height(0),
  m_innerLeft(0),
  m_innerTop(0),
  m_innerRight(0),
  m_innerBottom(0),
  m_innerWidth(0),
  m_innerHeight(0),
  m_drawHLine(true),
  m_drawVLine(true),
  m_drawCircle(true),
  m_pressed(false),
  m_xSelect(0),
  m_ySelect(0),
  m_callback(nullptr),
  m_bitmap(nullptr)
{
}

VSelectBox::VSelectBox(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VSelectBoxCallback callback) :
  VSelectBox()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VSelectBox::~VSelectBox()
{
  cleanup();
}

void VSelectBox::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VSelectBoxCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  parent.addChild(menuID, this);

  m_innerWidth = width;
  m_innerHeight = height;

  uint32_t borders = m_borderSize * 2;
  m_width = m_innerWidth + borders;
  m_height = m_innerHeight + borders;

  m_innerLeft = m_borderSize;
  m_innerTop = m_borderSize;
  // if the inner width/height is 1 (it's 1 pixel big) then it's the same 
  // pixel as the inner left/top. That helps to understand the -1 offset
  // because the inside size is 1x1 but the inner left == inner right and
  // the inner top == inner bottom because it's all the same 1x1 inner pixel
  m_innerRight = m_borderSize + m_innerWidth - 1;
  m_innerBottom = m_borderSize + m_innerHeight - 1;

  // create the window
  m_hwnd = CreateWindow(WC_HUE_SAT_BOX, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN ,
    x, y, m_width, m_height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  m_colorLabel.init(hInstance, parent, "", backcol, 100, 24, x + width + 6, y + (height / 4), 0, nullptr);
}

void VSelectBox::cleanup()
{
  //DeleteObject(m_bitmap);
}

void VSelectBox::create()
{
}

static HBRUSH getBrushCol(DWORD rgbcol)
{
  static std::map<COLORREF, HBRUSH> m_brushmap;
  HBRUSH br;
  COLORREF col = RGB((rgbcol >> 16) & 0xFF, (rgbcol >> 8) & 0xFF, rgbcol & 0xFF);
  if (m_brushmap.find(col) == m_brushmap.end()) {
    br = CreateSolidBrush(col);
    m_brushmap[col] = br;
  }
  br = m_brushmap[col];
  return br;
}

void VSelectBox::paint()
{
  PAINTSTRUCT paintStruct;
  memset(&paintStruct, 0, sizeof(paintStruct));
  HDC hdc = BeginPaint(m_hwnd, &paintStruct);
  RECT rect;
  GetClientRect(m_hwnd, &rect);
  // draw the data
  HDC compatDC = CreateCompatibleDC(hdc);
  HBITMAP hbmpOld = (HBITMAP)SelectObject(compatDC, m_bitmap);
  // copy the bitmap into the hdc
  BitBlt(hdc, 0, 0, m_width, m_height, compatDC, 0, 0, BLACKNESS);
  BitBlt(hdc, m_innerLeft, m_innerTop, m_innerWidth, m_innerHeight, compatDC, 0, 0, SRCCOPY);
  SelectObject(compatDC, hbmpOld);
  DeleteDC(compatDC);

  int selectorSize = 5;

  if (m_drawHLine) {
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    if (m_drawCircle) {
      MoveToEx(hdc, m_innerLeft, m_innerTop + m_ySelect, NULL);
      LineTo(hdc, m_innerLeft + (m_xSelect - selectorSize), m_innerTop + m_ySelect);
      MoveToEx(hdc, m_innerLeft + (m_xSelect + selectorSize), m_innerTop + m_ySelect, NULL);
      LineTo(hdc, m_innerLeft + m_innerWidth, m_innerTop + m_ySelect);
    } else {
      MoveToEx(hdc, m_innerLeft, m_innerTop + m_ySelect, NULL);
      LineTo(hdc, m_innerLeft + m_innerWidth, m_innerTop + m_ySelect);
    }
  }
  if (m_drawVLine) {
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    if (m_drawCircle) {
      MoveToEx(hdc, m_innerLeft + m_xSelect, m_innerTop, NULL);
      LineTo(hdc, m_innerLeft + m_xSelect, m_innerTop + (m_ySelect - selectorSize));
      MoveToEx(hdc, m_innerLeft + m_xSelect, m_innerTop + (m_ySelect + selectorSize), NULL);
      LineTo(hdc, m_innerLeft + m_xSelect, m_innerTop + m_innerHeight);
    } else {
      MoveToEx(hdc, m_innerLeft + m_xSelect, m_innerTop, NULL);
      LineTo(hdc, m_innerLeft + m_xSelect, m_innerTop + m_innerHeight);
    }
  }
  if (m_drawCircle) {
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Ellipse(hdc, m_innerLeft + (m_xSelect - selectorSize),
      m_innerTop + (m_ySelect - selectorSize),
      m_innerLeft + (m_xSelect + selectorSize) + 1,
      m_innerTop + (m_ySelect + selectorSize) + 1);
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Ellipse(hdc, m_innerLeft + (m_xSelect - selectorSize) + 1,
      m_innerTop + (m_ySelect - selectorSize) + 1,
      m_innerLeft + (m_xSelect + selectorSize),
      m_innerTop + (m_ySelect + selectorSize));
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    Ellipse(hdc, m_innerLeft + (m_xSelect - selectorSize) + 2,
      m_innerTop + (m_ySelect - selectorSize) + 2,
      m_innerLeft + (m_xSelect + selectorSize) - 1,
      m_innerTop + (m_ySelect + selectorSize) - 1);

  }

  EndPaint(m_hwnd, &paintStruct);
}

void VSelectBox::command(WPARAM wParam, LPARAM lParam)
{
}

void VSelectBox::pressButton()
{
  // Get the window client area.
  RECT rc;
  GetClientRect(m_hwnd, &rc);

  // Convert the client area to screen coordinates.
  POINT pt = { rc.left, rc.top };
  POINT pt2 = { rc.right, rc.bottom };
  ClientToScreen(m_hwnd, &pt);
  ClientToScreen(m_hwnd, &pt2);
  SetRect(&rc, pt.x, pt.y, pt2.x, pt2.y);

  SetCapture(m_hwnd);

  // Confine the cursor.
  ClipCursor(&rc);
  m_pressed = true;
}

void VSelectBox::releaseButton()
{
  ClipCursor(NULL);
  ReleaseCapture();
  m_pressed = false;
}

void VSelectBox::mouseMove()
{
  if (!m_pressed) {
    return;
  }
  POINT pos;
  GetCursorPos(&pos);
  ScreenToClient(m_hwnd, &pos);
  if (pos.x < m_innerLeft) pos.x = m_innerLeft;
  if (pos.x > m_innerRight) pos.x = m_innerRight;
  if (pos.y < m_innerTop) pos.y = m_innerTop;
  if (pos.y > m_innerBottom) pos.y = m_innerBottom;
  uint32_t innerX = pos.x - m_innerLeft;
  uint32_t innerY = pos.y - m_innerTop;
  setSelection(innerX, innerY);
  if (m_callback) {
    m_callback(m_callbackArg, innerX, innerY);
  }
}

void VSelectBox::setBackground(HBITMAP hBitmap)
{
  m_bitmap = hBitmap;
}

void VSelectBox::setSelection(uint32_t x, uint32_t y)
{
  m_xSelect = x;
  m_ySelect = y;
  //redraw();
}

LRESULT CALLBACK VSelectBox::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VSelectBox *pColorSelect = (VSelectBox *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
  case WM_MOUSEMOVE:
    pColorSelect->mouseMove();
    break;
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)pColorSelect->m_wc.hbrBackground;
  case WM_ERASEBKGND:
    return 1;
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

void VSelectBox::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VSelectBox::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VSelectBox::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_HUE_SAT_BOX;
  RegisterClass(&m_wc);
}