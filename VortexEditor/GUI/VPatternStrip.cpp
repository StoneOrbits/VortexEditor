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

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Msimg32.lib")

#define WC_PATTERN_STRIP "VPatternStrip"
#define LINE_SIZE 2

WNDCLASS VPatternStrip::m_wc = { 0 };

VPatternStrip::VPatternStrip() :
  VWindow(),
  d2dFactory(nullptr), renderTarget(nullptr), brush(nullptr),
  m_vortex(),
  m_runThread(nullptr),
  m_stripLabel(),
  m_callback(nullptr),
  m_active(true),
  m_selected(false),
  m_selectable(true),
  m_colorSequence(),
  m_lineWidth(1),
  m_numSlices(0),
  m_backbufferDC(nullptr),
  m_backbuffer(nullptr),
  m_oldBitmap(nullptr),
  m_backbufferWidth(0),
  m_backbufferHeight(0),
  m_scrollOffset(0)
{
}

VPatternStrip::VPatternStrip(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t lineWidth, const json &js, uintptr_t menuID, VPatternStripCallback callback) :
  VPatternStrip()
{
  init(hInstance, parent, title, backcol, width, height, x, y, lineWidth, js, menuID, callback);
}

VPatternStrip::~VPatternStrip()
{
  setActive(false);
  cleanup();
}

void VPatternStrip::init(HINSTANCE hInstance, VWindow &parent, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
  uint32_t lineWidth, const json &js, uintptr_t menuID, VPatternStripCallback callback)
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

  m_stripLabel.init(hInstance, parent, "", backcol, 400, 24, x + width + 6, y + (height / 4), 0, nullptr);
  //m_colorLabel.setVisible(false);

  // load the communty modes into the local vortex instance for the browser window
  m_vortex.init();
  m_vortex.setLedCount(1);
  m_vortex.setTickrate(1000);
  //m_vortex.setInstantTimestep(true);

  HDC hdc = GetDC(m_hwnd);
  createBackBuffer(hdc, width, height);

  loadJson(js);

  // Existing initialization logic...
  InitializeDirect2D(); // Initialize Direct2D components
}

void VPatternStrip::loadJson(const json &js)
{
  setActive(false);
  if (js.contains("modeData")) {
    if (js.contains("name")) {
      m_stripLabel.setText(js["name"]);
    }
    m_vortex.engine().modes().clearModes();
    m_vortex.loadModeFromJson(js["modeData"]);
  }

  //m_vortex.setInstantTimestep(true);
  //for (uint32_t i = 0; i < m_backbufferWidth; ++i) {
  //  m_vortex.engine().tick();
  //  RGBColor col = m_vortex.engine().leds().getLed(0);
  //  m_colorSequence.push_back(col);
  //  if (m_colorSequence.size() > m_numSlices) {
  //    m_colorSequence.pop_front();
  //  }
  //  // Update scroll offset for scrolling animation
  //  m_scrollOffset = (m_scrollOffset + 1) % m_numSlices;
  //}
  //m_vortex.setInstantTimestep(false);

}

DWORD __stdcall VPatternStrip::runThread(void *arg)
{
  VPatternStrip *strip = (VPatternStrip *)arg;
  while (strip->m_active) {
    strip->run();
  }
  return 0;
}

void VPatternStrip::cleanup()
{
  destroyBackBuffer();
}

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
  UpdateWindow(m_hwnd);
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

// Initialize Direct2D resources
bool VPatternStrip::InitializeDirect2D()
{
  HRESULT hr;
  if (!d2dFactory) {
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
    if (FAILED(hr)) {
      return false;
    }
  }
  if (!renderTarget) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    hr = d2dFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hwnd, size),
      &renderTarget
    );
    if (FAILED(hr)) {
      return false;
    }

    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
    if (FAILED(hr)) {
      return false;
    }
  }
  return true;
}

// Clean up Direct2D resources
void VPatternStrip::DiscardDirect2DResources()
{
  if (brush) { brush->Release(); brush = nullptr; }
  if (renderTarget) { renderTarget->Release(); renderTarget = nullptr; }
  if (d2dFactory) { d2dFactory->Release(); d2dFactory = nullptr; }
}

void VPatternStrip::paint()
{
  if (!renderTarget) {
    InitializeDirect2D(); // Ensure Direct2D is initialized
    if (!renderTarget) return; // Fail if still not initialized
  }

  renderTarget->BeginDraw();
  renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));  // Clearing the background

  // Assuming m_colorSequence holds the colors for each segment or line
  const float segmentWidth = static_cast<float>(m_backbufferWidth) / m_colorSequence.size();
  float currentX = 0;

  for (const auto &color : m_colorSequence) {
    // Set the brush color for each segment
    D2D1_COLOR_F d2dColor = D2D1::ColorF(GetRValue(color.raw()), GetGValue(color.raw()), GetBValue(color.raw()));
    brush->SetColor(d2dColor);

    // Define the rectangle for the current segment
    D2D1_RECT_F rect = D2D1::RectF(currentX, 0, currentX + segmentWidth, static_cast<float>(m_backbufferHeight));

    // Fill the rectangle with the current color
    renderTarget->FillRectangle(&rect, brush);

    // Move to the next segment
    currentX += segmentWidth;
  }

  HRESULT hr = renderTarget->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    DiscardDirect2DResources();
    InitializeDirect2D();
  } else if (FAILED(hr)) {
    MessageBox(nullptr, "Failed to draw Direct2D content.", "Draw Error", MB_OK);
  }
}


void VPatternStrip::drawToBackBuffer()
{
  if (!m_backbufferDC) {
    return;
  }

  RECT rect;
  GetClientRect(m_hwnd, &rect);
  uint32_t width = rect.right - rect.left;
  uint32_t height = rect.bottom - rect.top;

  // Fill the backbuffer background
  FillRect(m_backbufferDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

  // Calculate the starting position for drawing lines
  uint32_t numLines = m_colorSequence.size();
  int startX = width - (numLines * m_lineWidth) % width;
  RECT lineRect;
  lineRect.top = rect.top;
  lineRect.bottom = rect.bottom;

  // Iterate through each color in the sequence and draw the corresponding line
  for (int i = 0; i < numLines; ++i) {
    const auto &col = m_colorSequence[i];
    if (col.empty()) {
      continue;
    }

    HBRUSH brush = getBrushCol(col.raw());
    if (!brush) {
      continue;
    }

    int xPos = (startX + (i * m_lineWidth)) % width;
    lineRect.left = xPos;
    lineRect.right = xPos + m_lineWidth;

    FillRect(m_backbufferDC, &lineRect, brush);
  }
}

void VPatternStrip::command(WPARAM wParam, LPARAM lParam)
{
}

void VPatternStrip::pressButton(WPARAM wParam, LPARAM lParam)
{
  g_pEditor->addMode(nullptr, m_vortex.engine().modes().curMode());
  //m_vortex.setTickrate(100);
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
  RECT rect;
  GetClientRect(m_hwnd, &rect);
  FillRect(m_backbufferDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
  m_colorSequence.clear();
  InvalidateRect(m_hwnd, nullptr, FALSE);
}

bool VPatternStrip::isActive() const
{
  return m_active;
}

void VPatternStrip::setActive(bool active)
{
  m_active = active;
  m_stripLabel.setVisible(active);

  // if active == false
  if (!m_active) {
    setSelected(false);
    if (m_runThread) {
      WaitForSingleObject(m_runThread, INFINITE);
      m_runThread = nullptr;
    }
    clear();
    //  end active == false
    return;
  }

  // otherwise active = true
  if (!m_runThread) {
    m_runThread = CreateThread(NULL, 0, runThread, this, 0, NULL);
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
    m_stripLabel.setForeColor(0xFFFFFF);
  } else {
    m_stripLabel.setForeColor(0xAAAAAA);
  }
}

void VPatternStrip::setLabelEnabled(bool enabled)
{
  m_stripLabel.setVisible(enabled);
  m_stripLabel.setEnabled(enabled);
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
    //SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
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

void VPatternStrip::updateVisuals()
{
  if (!m_backbufferDC) return;

  // Clear the backbuffer with the background color
  RECT backbufferRect = { 0, 0, m_backbufferWidth, m_backbufferHeight };
  FillRect(m_backbufferDC, &backbufferRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

  // Implement your drawing logic here, similar to the existing drawToBackBuffer logic
  // This might involve iterating over m_colorSequence and drawing each color
  int xPos = 0;
  for (const auto &color : m_colorSequence) {
    HBRUSH brush = getBrushCol(color.raw());
    RECT lineRect = { xPos, 0, xPos + m_lineWidth, m_backbufferHeight };
    FillRect(m_backbufferDC, &lineRect, brush);
    xPos += m_lineWidth;
    if (xPos >= m_backbufferWidth) break; // Stop if we've filled the whole backbuffer
  }
}

void VPatternStrip::draw(HDC hdc, int x, int y, int width, int height)
{
  updateVisuals(); // Make sure visuals are up-to-date before drawing

  // Use BitBlt to transfer the backbuffer content to the specified device context.
  // Adjust source and destination rectangles if you want to support scaling or partial drawing.
  BitBlt(hdc, x, y, width, height, m_backbufferDC, 0, 0, SRCCOPY);
}

//void VPatternStrip::paint()
//{
//  PAINTSTRUCT ps;
//  HDC hdc = BeginPaint(m_hwnd, &ps);
//
//  updateVisuals(); // Update the backbuffer first
//
//  // Now draw the backbuffer to this window's client area.
//  BitBlt(hdc, 0, 0, m_backbufferWidth, m_backbufferHeight, m_backbufferDC, 0, 0, SRCCOPY);
//
//  EndPaint(m_hwnd, &ps);
//}

// Remember to implement createBackBuffer and destroyBackBuffer methods as well.
// These methods will handle the creation and destruction of the backbuffer resources.

void VPatternStrip::createBackBuffer(HDC hdc, uint32_t width, uint32_t height)
{
  if (m_backbufferDC) {
    // If there's already a backbuffer, release it first
    destroyBackBuffer();
  }

  m_backbufferDC = CreateCompatibleDC(hdc);
  m_backbuffer = CreateCompatibleBitmap(hdc, width, height);
  m_oldBitmap = (HBITMAP)SelectObject(m_backbufferDC, m_backbuffer);
  m_backbufferWidth = width;
  m_backbufferHeight = height;
}

void VPatternStrip::destroyBackBuffer()
{
  if (m_backbufferDC) {
    SelectObject(m_backbufferDC, m_oldBitmap); // Restore the old bitmap
    DeleteDC(m_backbufferDC);
    DeleteObject(m_backbuffer);
    m_backbufferDC = nullptr;
    m_backbuffer = nullptr;
  }
}

