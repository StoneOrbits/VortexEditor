#include "VPatternStrip.h"

// Windows includes
#include <CommCtrl.h>
#include <Windowsx.h>

// Vortex Engine includes
#include "EditorConfig.h"
#include "Colors/ColorTypes.h"

// Editor includes
#include "VortexEditor.h"

using namespace std;

#pragma comment(lib, "Msimg32.lib");

#define WC_PATTERN_STRIP "VPatternStrip"
#define LINE_SIZE 2

WNDCLASS VPatternStrip::m_wc = { 0 };

VPatternStrip::VPatternStrip() :
  VWindow(),
  m_vortex(),
  m_colorLabel(),
  m_callback(nullptr),
  m_color(0),
  m_active(true),
  m_selected(false),
  m_selectable(true),
  m_colorSequence()
{
}

VPatternStrip::VPatternStrip(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t lineWidth, const json &modeData, uintptr_t menuID, VPatternStripCallback callback) :
  VPatternStrip()
{
  init(hInstance, parent, title, backcol, width, height, x, y, lineWidth, modeData, menuID, callback);
}

VPatternStrip::~VPatternStrip()
{
  cleanup();
}

void VPatternStrip::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t lineWidth, const json &modeData, uintptr_t menuID, VPatternStripCallback callback)
{
  // store callback and menu id
  m_callback = callback;
  m_backColor = backcol;
  m_foreColor = RGB(0xD0, 0xD0, 0xD0);
  m_numSlices = width / LINE_SIZE;
  m_lineWidth = lineWidth;

  // register window class if it hasn't been registered yet
  registerWindowClass(hInstance, backcol);

  if (!menuID) {
    menuID = nextMenuID++;
  }

  if (!parent.addChild(menuID, this)) {
    return;
  }

  // create the window
  m_hwnd = CreateWindow(WC_PATTERN_STRIP, title.c_str(),
    WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
    x, y, width, height, parent.hwnd(), (HMENU)menuID, nullptr, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  m_colorLabel.init(hInstance, parent, title, backcol, 100, 24, x + width + 6, y + (height / 4), 0, nullptr);

  //// preview box for current color
  //m_colorPreview.init(hInst, m_communityBrowserWindow, "", BACK_COL, 122, 96, 273, 274, PREVIEW_ID, clickCurColorCallback);
  //m_colorPreview.setActive(true);
  //m_colorPreview.setColor(0xFF0000);
  //m_colorPreview.setSelectable(false);

  // load the communty modes into the local vortex instance for the browser window
  m_vortex.init();
  m_vortex.setLedCount(1);
  m_vortex.setTickrate(30);
  m_vortex.engine().modes().clearModes();
  m_vortex.loadModeFromJson(modeData);
}

void VPatternStrip::cleanup()
{
}

uint32_t m_scrollOffset = 0;

void VPatternStrip::run()
{
  m_vortex.engine().tick();
  RGBColor col = m_vortex.engine().leds().getLed(0);
  m_colorSequence.push_back(col);
  if (m_colorSequence.size() > m_numSlices) {
    m_colorSequence.pop_front();
  }

  // Update scroll offset for scrolling animation
  m_scrollOffset = (m_scrollOffset + 1) % m_numSlices;

  // Trigger window update for animation
  InvalidateRect(m_hwnd, nullptr, FALSE);
}

void VPatternStrip::create()
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

void VPatternStrip::paint()
{
  PAINTSTRUCT paintStruct;
  HDC hdc = BeginPaint(m_hwnd, &paintStruct);
  RECT rect;
  GetClientRect(m_hwnd, &rect);
  uint32_t width = rect.right - rect.left;
  uint32_t height = rect.bottom - rect.top;

  // Create an off-screen buffer to perform our drawing operations
  HDC backbuffDC = CreateCompatibleDC(hdc);
  HBITMAP backbuffer = CreateCompatibleBitmap(hdc, width, height);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(backbuffDC, backbuffer);

  // Initially fill the background
  FillRect(backbuffDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

  // Draw the new frame on top
  uint32_t numLines = m_colorSequence.size(); // Number of lines we have in our sequence

  // Calculate the starting x position for the first line (right-most)
  int startX = width - (numLines * m_lineWidth) % width;

  for (int i = 0; i < numLines; ++i) {
    // Determine the color for this line
    RGBColor col = m_colorSequence[i];
    if (col.empty()) {
      continue;
    }
    HBRUSH brush = getBrushCol(col.raw());
    if (!brush) {
      continue;
    }
    // draw the line
    int xPos = (startX + (i * m_lineWidth)) % width;
    RECT lineRect = { xPos, rect.top, xPos + m_lineWidth, rect.bottom };
    FillRect(backbuffDC, &lineRect, brush);
  }

  // Create a temporary DC for the darkening operation
  HDC tempDC = CreateCompatibleDC(hdc);
  HBITMAP tempBitmap = CreateCompatibleBitmap(hdc, width, height);
  SelectObject(tempDC, tempBitmap);

  // Copy current backbuffer content to tempDC
  BitBlt(tempDC, 0, 0, width, height, backbuffDC, 0, 0, SRCCOPY);

  BLENDFUNCTION blendFunc = { 0 };
  blendFunc.BlendOp = AC_SRC_OVER;
  blendFunc.SourceConstantAlpha = 230; // Adjust for desired trail darkness
  blendFunc.AlphaFormat = 0;

  // Apply the darkening effect on the backbuffer using content from tempDC
  AlphaBlend(backbuffDC, 0, 0, width, height, tempDC, 0, 0, width, height, blendFunc);

  // Cleanup temporary objects
  DeleteDC(tempDC);
  DeleteObject(tempBitmap);

  // Copy the off-screen buffer to the screen
  BitBlt(hdc, 0, 0, width, height, backbuffDC, 0, 0, SRCCOPY);

  // Cleanup
  SelectObject(backbuffDC, oldBitmap);
  DeleteObject(backbuffer);
  DeleteDC(backbuffDC);
  EndPaint(m_hwnd, &paintStruct);
}

void VPatternStrip::command(WPARAM wParam, LPARAM lParam)
{
}

void VPatternStrip::pressButton(WPARAM wParam, LPARAM lParam)
{
  //if (m_selectable) {
  //  setSelected(!m_selected);
  //  if (m_selected && !m_active) {
  //    // if the box was inactive and just selected, activate it
  //    setActive(true);
  //    clear();
  //  }
  //}
  //SelectEvent sevent = SELECT_LEFT_CLICK;
  //if (wParam & MK_CONTROL) {
  //  sevent = SELECT_CTRL_LEFT_CLICK;
  //}
  //if (wParam & MK_SHIFT) {
  //  sevent = SELECT_SHIFT_LEFT_CLICK;
  //}
  //if (m_callback) {
  //  m_callback(m_callbackArg, this, sevent);
  //}
}

void VPatternStrip::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

// window message for right button press, only exists here
void VPatternStrip::rightButtonPress()
{
  //if (m_selectable) {
  //  if (m_selected) {
  //    setSelected(false);
  //  } else {
  //    if (m_color == 0) {
  //      setActive(false);
  //    }
  //    clear();
  //  }
  //}
  //if (m_callback) {
  //  m_callback(m_callbackArg, this, SELECT_RIGHT_CLICK);
  //}
}

void VPatternStrip::clear()
{
  setColor(0);
}

void VPatternStrip::setColor(uint32_t col)
{
  m_color = col;
  m_colorLabel.setText(getColorName());
  redraw();
}

string VPatternStrip::getColorName() const
{
  if (m_color == 0) {
    return "blank";
  }
  char colText[64] = { 0 };
  snprintf(colText, sizeof(colText), "#%02X%02X%02X",
    (m_color >> 16) & 0xFF, (m_color >> 8) & 0xFF, m_color & 0xFF);
  return colText;
}

void VPatternStrip::setColor(std::string name)
{
  if (name == "blank") {
    setColor(0);
    return;
  }
  // either the start of string, or string + 1 if the first
  // letter is a hashtag/pound character
  const char *hexStr = name.c_str() + (name[0] == '#');
  setColor(strtoul(hexStr, NULL, 16));
}

uint32_t VPatternStrip::getColor() const
{
  return m_color;
}

bool VPatternStrip::isActive() const
{
  return m_active;
}

void VPatternStrip::setActive(bool active)
{
  m_active = active;
  m_colorLabel.setVisible(active);
  if (!m_active) {
    setSelected(false);
  }
}

bool VPatternStrip::isSelected() const
{
  return m_selected;
}

void VPatternStrip::setSelected(bool selected)
{
  m_selected = selected;
  if (m_selected) {
    m_colorLabel.setForeColor(0xFFFFFF);
  } else {
    m_colorLabel.setForeColor(0xAAAAAA);
  }
}

void VPatternStrip::setLabelEnabled(bool enabled)
{
  m_colorLabel.setVisible(enabled);
  m_colorLabel.setEnabled(enabled);
}

void VPatternStrip::setSelectable(bool selectable)
{
  m_selectable = selectable;
  if (!m_selectable) {
    setSelected(false);
  }
}

LRESULT CALLBACK VPatternStrip::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VPatternStrip *pPatternStrip = (VPatternStrip *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!pPatternStrip) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  switch (uMsg) {
  case WM_VSCROLL:
    break;
  case WM_LBUTTONDOWN:
    pPatternStrip->pressButton(wParam, lParam);
    break;
  case WM_LBUTTONUP:
    pPatternStrip->releaseButton(wParam, lParam);
    break;
  case WM_RBUTTONUP:
    pPatternStrip->rightButtonPress();
    break;
  case WM_KEYDOWN:
    // TODO: implement key control of color select?
    break;
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)pPatternStrip->m_wc.hbrBackground;
  case WM_CREATE:
    pPatternStrip->create();
    break;
  case WM_PAINT:
    pPatternStrip->paint();
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_COMMAND:
    pPatternStrip->command(wParam, lParam);
    break;
  case WM_DESTROY:
    pPatternStrip->cleanup();
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void VPatternStrip::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VPatternStrip::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VPatternStrip::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
  m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_wc.lpszClassName = WC_PATTERN_STRIP;
  RegisterClass(&m_wc);
}
