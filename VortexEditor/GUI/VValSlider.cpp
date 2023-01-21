#include "VValSlider.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"
#include "Colors/ColorTypes.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

#define WC_VAL_SLIDER "VValSlider"

#define PI 3.141592654

WNDCLASS VValSlider::m_wc = {0};

VValSlider::VValSlider() :
  VWindow(),
  m_borderSize(2),
  m_callback(nullptr),
  m_bitmap(nullptr)
{
}

VValSlider::VValSlider(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VValSlider()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VValSlider::~VValSlider()
{
  cleanup();
}

void VValSlider::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  parent.addChild(menuID, this);

  uint32_t borders = m_borderSize * 2;
  m_width = width + borders;
  m_height = height + borders;

  // create the window
  m_hwnd = CreateWindow(WC_VAL_SLIDER, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
    x, y, m_width, m_height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  m_colorLabel.init(hInstance, parent, "", backcol, 100, 24, x + width + 6, y + (height / 4), 0, nullptr);

  HDC hDC = GetDC(m_hwnd);
  HDC memDC = CreateCompatibleDC(hDC);
  m_bitmap = CreateCompatibleBitmap(hDC, m_width, m_height);
  HGDIOBJ oldObj = SelectObject(memDC, m_bitmap);

  int hue = 0;
  int sat = 255;
  int val = 255;
  for (uint32_t y = 0; y < m_height; ++y) {
    RGBColor rgbCol = hsv_to_rgb_rainbow(HSVColor(hue, sat, val));
    COLORREF col = ((rgbCol.blue << 16) | (rgbCol.green << 8) | (rgbCol.red));
    for (uint32_t x = 0; x < m_width; ++x) {
      if (x < m_borderSize  || y < m_borderSize || x >= ((m_width - m_borderSize)) || y >= ((m_height - m_borderSize))) { 
        SetPixel(memDC, x, y, 0);
        continue;
      }
      SetPixel(memDC, x, y, col);
    }
    val--;
  }
  SelectObject(memDC, oldObj);
  DeleteDC(memDC);
}

void VValSlider::cleanup()
{
  DeleteObject(m_bitmap);
}

void VValSlider::create()
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

void VValSlider::paint()
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
  BitBlt(hdc, 0, 0, m_width, m_height, compatDC, 0, 0, SRCCOPY);
  SelectObject(compatDC, hbmpOld);
  DeleteDC(compatDC);
  
  int selectorSize = 10;

  SelectObject(hdc, GetStockObject(BLACK_PEN));
  MoveToEx(hdc, 0, m_valPos, NULL);
  LineTo(hdc, m_width, m_valPos);

  EndPaint(m_hwnd, &paintStruct);
}

void VValSlider::command(WPARAM wParam, LPARAM lParam)
{
}

void VValSlider::pressButton()
{
  POINT pos;
  GetCursorPos(&pos);
  ScreenToClient(m_hwnd, &pos);
  m_valPos = pos.y;
  RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
#if 0
  CHOOSECOLOR col;
  memset(&col, 0, sizeof(col));
  ZeroMemory(&col, sizeof(col));
  col.lStructSize = sizeof(col);
  col.hwndOwner = m_hwnd;
  static COLORREF acrCustClr[16]; // array of custom colors 
  col.lpCustColors = (LPDWORD)acrCustClr;
  // windows uses BGR
  col.rgbResult = getFlippedColor();
  col.Flags = CC_FULLOPEN | CC_RGBINIT;
  ChooseColor(&col);
  // flip the result back from BGR to RGB
  setFlippedColor(col.rgbResult);
  setActive(true);
  m_callback(m_callbackArg, this);
#endif
}

void VValSlider::releaseButton()
{
}

// window message for right button press, only exists here
void VValSlider::rightButtonPress()
{
#if 0
  if (m_color == 0) {
    setActive(false);
  }
  clear();
  m_callback(m_callbackArg, this);
#endif
}

void VValSlider::clear()
{
  setColor(0);
}

void VValSlider::setColor(uint32_t col)
{
  //m_color = col;
  //m_colorLabel.setText(getColorName());
  //RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

string VValSlider::getColorName() const
{
  //if (m_color == 0) {
    return "blank";
  //}
  //char colText[64] = { 0 };
  //snprintf(colText, sizeof(colText), "#%02X%02X%02X",
  //  (m_color >> 16) & 0xFF, (m_color >> 8) & 0xFF, m_color & 0xFF);
  //return colText;
}

void VValSlider::setColor(std::string name)
{
  if (name == "blank") {
    setColor(0);
    return;
  }
  if (name[0] != '#') {
    // ??
    return;
  }
  setColor(strtoul(name.c_str() + 1, NULL, 16));
}

void VValSlider::setFlippedColor(uint32_t col)
{
  setColor(((col >> 16) & 0xFF) | (col & 0xFF00) | ((col << 16) & 0xFF0000));
}

uint32_t VValSlider::getColor() const
{
  return 0; //m_color;
}

uint32_t VValSlider::getFlippedColor() const
{
  return 0; //((m_color >> 16) & 0xFF) | (m_color & 0xFF00) | ((m_color << 16) & 0xFF0000);
}

bool VValSlider::isActive() const
{
  return 0; //m_active;
}

void VValSlider::setActive(bool active)
{
  //m_active = active;
  //m_colorLabel.setVisible(active);
}

LRESULT CALLBACK VValSlider::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VValSlider *pColorSelect = (VValSlider *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
  case WM_RBUTTONUP:
    pColorSelect->rightButtonPress();
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

void VValSlider::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VValSlider::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VValSlider::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_VAL_SLIDER;
  RegisterClass(&m_wc);
}