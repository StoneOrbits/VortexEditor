#include "VColorRing.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"
#include "Colors/ColorTypes.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

#define WC_COLOR_RING "VColorRing"

#define PI 3.141592654

WNDCLASS VColorRing::m_wc = {0};

VColorRing::VColorRing() :
  VWindow(),
  m_callback(nullptr),
  m_color(0),
  m_active(false),
  m_radius(0),
  m_bitmap(nullptr)
{
}

VColorRing::VColorRing(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback) :
  VColorRing()
{
  init(hInstance, parent, title, backcol, width, height, x, y, menuID, callback);
}

VColorRing::~VColorRing()
{
  cleanup();
}

void VColorRing::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uintptr_t menuID, VWindowCallback callback)
{
  // store callback and menu id
  m_callback = callback;

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  parent.addChild(menuID, this);

  // create the window
  m_hwnd = CreateWindow(WC_COLOR_RING, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
    x, y, width + 2, height + 2, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  m_radius = (width / 2);

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  m_colorLabel.init(hInstance, parent, "", backcol, 100, 24, x + width + 6, y + (height / 4), 0, nullptr);

#if 0
  for (int x = 0; x <= (m_radius * 2); ++x) {
    for (int y = 0; y <= (m_radius * 2); ++y) {
      // adjacent and opposite
      int adj = m_radius - x;
      int opp = m_radius - y;
      // (x, y) -> (r, r)
      double dist = sqrt((adj * adj) + (opp * opp));
      if (dist >= m_radius) {
        if (dist <= m_radius + 2) {
          double diff = 2 - (dist - m_radius);
          m_bitmap[(y * width) + x] = 0;
        }
        continue;
      }
      // calculate hue
      int hue = atan2(opp, -adj) * (255 / 360.0) * (180.0 / PI);
      // calculate sat
      int sat = 20 + ((235.0 * dist) / m_radius);
      // calculate val
      //int val = 200 + ((50.0 * dist) / m_radius);
      HSVColor hsvCol = HSVColor(hue, sat, 255);
#define SCALE8(i, scale)  (((uint16_t)i * (uint16_t)(scale)) >> 8)
      //hsvCol.hue = SCALE8(hsvCol.hue, 191);
      RGBColor rgbCol = hsv_to_rgb_rainbow(hsvCol);
      COLORREF col = ((rgbCol.blue << 16) | (rgbCol.green << 8) | (rgbCol.red));
      m_bitmap[(y * width) + x] = col;
    }
  }
#endif
}

void VColorRing::cleanup()
{
}

void VColorRing::create()
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

void VColorRing::paint()
{
  PAINTSTRUCT paintStruct;
  memset(&paintStruct, 0, sizeof(paintStruct));
  HDC hdc = BeginPaint(m_hwnd, &paintStruct);
  RECT rect;
  GetClientRect(m_hwnd, &rect);
  if (m_active) {
    for (int x = 0; x <= (m_radius * 2); ++x) {
      for (int y = 0; y <= (m_radius * 2); ++y) {
        // adjacent and opposite
        int adj = m_radius - x;
        int opp = m_radius - y;
        // (x, y) -> (r, r)
        double dist = sqrt((adj * adj) + (opp * opp));
        if (dist >= m_radius) {
          if (dist <= m_radius + 2) {
            double diff = 2 - (dist - m_radius);
            SetPixel(hdc, x, y, 0);
          }
          continue;
        }
        // calculate hue
        int hue = atan2(opp, -adj) * (255 / 360.0) * (180.0 / PI);
        // calculate sat
        int sat = 255; //20 + ((235.0 * dist) / m_radius);
        // calculate val
        //int val = 200 + ((50.0 * dist) / m_radius);
        HSVColor hsvCol = HSVColor(hue, sat, 255);
#define SCALE8(i, scale)  (((uint16_t)i * (uint16_t)(scale)) >> 8)
        //hsvCol.hue = SCALE8(hsvCol.hue, 191);
        RGBColor rgbCol = hsv_to_rgb_rainbow(hsvCol);
        COLORREF col = ((rgbCol.blue << 16) | (rgbCol.green << 8) | (rgbCol.red));
        SetPixel(hdc, x, y, col);
      }
    }
  }
  EndPaint(m_hwnd, &paintStruct);
}

void VColorRing::command(WPARAM wParam, LPARAM lParam)
{
}

void VColorRing::pressButton()
{
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

void VColorRing::releaseButton()
{
}

// window message for right button press, only exists here
void VColorRing::rightButtonPress()
{
#if 0
  if (m_color == 0) {
    setActive(false);
  }
  clear();
  m_callback(m_callbackArg, this);
#endif
}

void VColorRing::clear()
{
  setColor(0);
}

void VColorRing::setColor(uint32_t col)
{
  m_color = col;
  m_colorLabel.setText(getColorName());
  RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

string VColorRing::getColorName() const
{
  if (m_color == 0) {
    return "blank";
  }
  char colText[64] = { 0 };
  snprintf(colText, sizeof(colText), "#%02X%02X%02X",
    (m_color >> 16) & 0xFF, (m_color >> 8) & 0xFF, m_color & 0xFF);
  return colText;
}

void VColorRing::setColor(std::string name)
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

void VColorRing::setFlippedColor(uint32_t col)
{
  setColor(((col >> 16) & 0xFF) | (col & 0xFF00) | ((col << 16) & 0xFF0000));
}

uint32_t VColorRing::getColor() const
{
  return m_color;
}

uint32_t VColorRing::getFlippedColor() const
{
  return ((m_color >> 16) & 0xFF) | (m_color & 0xFF00) | ((m_color << 16) & 0xFF0000);
}

bool VColorRing::isActive() const
{
  return m_active;
}

void VColorRing::setActive(bool active)
{
  m_active = active;
  m_colorLabel.setVisible(active);
}

LRESULT CALLBACK VColorRing::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VColorRing *pColorSelect = (VColorRing *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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

void VColorRing::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VColorRing::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VColorRing::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_COLOR_RING;
  RegisterClass(&m_wc);
}