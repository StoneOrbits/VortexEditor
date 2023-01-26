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
  m_color(0),
  m_active(false),
  m_selected(false)
{
}

VColorSelect::VColorSelect(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VColorSelectCallback callback) :
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
  uintptr_t menuID, VColorSelectCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_COLOR_SELECT, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP | CS_DBLCLKS,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  m_colorLabel.init(hInstance, parent, "", backcol, 100, 24, x + width + 6, y + (height / 4), 0, nullptr);
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
  COLORREF col = RGB((rgbcol >> 16) & 0xFF, (rgbcol >> 8) & 0xFF, rgbcol & 0xFF);
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

  uint32_t width = rect.right - rect.left;
  uint32_t height = rect.bottom - rect.top;

  // create a backbuffer and select it
  HDC backbuffDC = CreateCompatibleDC(hdc);
  HBITMAP backbuffer = CreateCompatibleBitmap(hdc, width, height);
  SelectObject(backbuffDC, backbuffer);

  COLORREF frontCol = getColor();
  COLORREF borderCol;

  if (m_selected) {
    borderCol = 0xFFFFFF; // white
  } else {
    if (m_active) {
      // border is bright if a color is selected, or dark if 'blank' (black)
      borderCol = frontCol ? 0xAAAAAA : 0x606060;
    } else {
      // force the frontcol to 0 if not active, and red border
      borderCol = 0xFF0000;
      frontCol = 0;
    }
  }
  FillRect(backbuffDC, &rect, getBrushCol(borderCol));

#define BORDER_WIDTH 1
  rect.left += BORDER_WIDTH;
  rect.top += BORDER_WIDTH;
  rect.right -= BORDER_WIDTH;
  rect.bottom -= BORDER_WIDTH;
  FillRect(backbuffDC, &rect, getBrushCol(frontCol));

  BitBlt(hdc, 0, 0, width, height, backbuffDC, 0, 0, SRCCOPY);

  DeleteObject(backbuffer);
  DeleteDC(backbuffDC);

  EndPaint(m_hwnd, &paintStruct);
}

void VColorSelect::command(WPARAM wParam, LPARAM lParam)
{
}

void VColorSelect::pressButton(WPARAM wParam, LPARAM lParam)
{
  if (!m_callback) {
    return;
  }
  //CHOOSECOLOR col;
  //memset(&col, 0, sizeof(col));
  //ZeroMemory(&col, sizeof(col));
  //col.lStructSize = sizeof(col);
  //col.hwndOwner = m_hwnd;
  //static COLORREF acrCustClr[16]; // array of custom colors 
  //col.lpCustColors = (LPDWORD)acrCustClr;
  //// windows uses BGR
  //col.rgbResult = getFlippedColor();
  //col.Flags = CC_FULLOPEN | CC_RGBINIT;
  //ChooseColor(&col);
  //// flip the result back from BGR to RGB
  //setFlippedColor(col.rgbResult);
  //setActive(true);
  m_selected = !m_selected;
  if (m_selected && !m_active) {
    // if the box was inactive and just selected, activate it
    setActive(true);
    clear();
  }
  SelectEvent sevent = SELECT_LEFT_CLICK;
  if (wParam & MK_CONTROL) {
    sevent = SELECT_CTRL_LEFT_CLICK;
  }
  if (wParam & MK_SHIFT) {
    sevent = SELECT_SHIFT_LEFT_CLICK;
  }
  m_callback(m_callbackArg, this, sevent);
}

void VColorSelect::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

// window message for right button press, only exists here
void VColorSelect::rightButtonPress()
{
  if (m_selected) {
    m_selected = false;
  } else {
    if (m_color == 0) {
      setActive(false);
    }
    clear();
  }
  m_callback(m_callbackArg, this, SELECT_RIGHT_CLICK);
}

void VColorSelect::clear()
{
  setColor(0);
}

void VColorSelect::setColor(uint32_t col)
{
  m_color = col;
  m_colorLabel.setText(getColorName());
  redraw();
}

string VColorSelect::getColorName() const
{
  if (m_color == 0) {
    return "blank";
  }
  char colText[64] = { 0 };
  snprintf(colText, sizeof(colText), "#%02X%02X%02X",
    (m_color >> 16) & 0xFF, (m_color >> 8) & 0xFF, m_color & 0xFF);
  return colText;
}

void VColorSelect::setColor(std::string name)
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

void VColorSelect::setFlippedColor(uint32_t col)
{
  setColor(((col >> 16) & 0xFF) | (col & 0xFF00) | ((col << 16) & 0xFF0000));
}

uint32_t VColorSelect::getColor() const
{
  return m_color;
}

uint32_t VColorSelect::getFlippedColor() const
{
  return ((m_color >> 16) & 0xFF) | (m_color & 0xFF00) | ((m_color << 16) & 0xFF0000);
}

bool VColorSelect::isActive() const
{
  return m_active;
}

void VColorSelect::setActive(bool active)
{
  m_active = active;
  m_colorLabel.setVisible(active);
  if (!m_active) {
    setSelected(false);
  }
}

bool VColorSelect::isSelected() const
{
  return m_selected;
}

void VColorSelect::setSelected(bool selected)
{
  m_selected = selected;
}

void VColorSelect::setLabelEnabled(bool enabled)
{
  m_colorLabel.setVisible(enabled);
  m_colorLabel.setEnabled(enabled);
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
    pColorSelect->pressButton(wParam, lParam);
    break;
  case WM_LBUTTONUP:
    pColorSelect->releaseButton(wParam, lParam);
    break;
  case WM_RBUTTONUP:
    pColorSelect->rightButtonPress();
    break;
  case WM_KEYDOWN:
    printf("Keydown\n");
    break;
  case WM_LBUTTONDBLCLK:
    printf("DCLICK\n");
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
  m_wc.lpfnWndProc = VColorSelect::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_COLOR_SELECT;
  RegisterClass(&m_wc);
}
