#include "VChildWindow.h"

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

#include <CommCtrl.h>
#include <Dbt.h>

#include "resource.h"

using namespace std;

WNDCLASS VChildWindow::m_wc = {0};

VChildWindow::VChildWindow() :
  VWindow()
{
}

VChildWindow::VChildWindow(HINSTANCE hinstance, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height,
  void *callbackArg) :
  VChildWindow()
{
  init(hinstance, title, backcol, width, height, callbackArg);
}

VChildWindow::~VChildWindow()
{
  cleanup();
}

void VChildWindow::init(HINSTANCE hInstance, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height,
  void *callbackArg)
{
  // store callback
  m_callbackArg = callbackArg;

  // register a window class for the window if not done yet
  registerWindowClass(hInstance, backcol);

  // get desktop rect so we can center the window
  RECT desktop;
  GetClientRect(GetDesktopWindow(), &desktop);

  // create the window
  m_hwnd = CreateWindow(WC_VCHILDWINDOW, title.c_str(),
    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
    (desktop.right / 2) - (width / 2), (desktop.bottom / 2) - (height / 2),
    width, height, nullptr, nullptr, hInstance, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, "Failed to open window", "Error", 0);
    throw exception("idk");
  }

  // set 'this' in the user data area of the class so that the static callback
  // routine can access the object
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void VChildWindow::cleanup()
{
}

bool VChildWindow::process(MSG &msg)
{
  return IsDialogMessage(m_hwnd, &msg);
}

void VChildWindow::create()
{
}

void VChildWindow::paint()
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(m_hwnd, &ps);
  EndPaint(m_hwnd, &ps);
}

void VChildWindow::pressButton(WPARAM wParam, LPARAM lParam)
{
}

void VChildWindow::releaseButton(WPARAM wParam, LPARAM lParam)
{
}

void VChildWindow::setVisible(bool visible)
{
  VWindow::setVisible(visible);
  if (!visible && m_closeCallback) {
    m_closeCallback(m_callbackArg, this);
  }
}

void VChildWindow::setCloseCallback(VWindowCallback callback)
{
  m_closeCallback = callback;
}

LRESULT CALLBACK VChildWindow::window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VChildWindow *pWindow = (VChildWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (!pWindow) {
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  switch (uMsg) {
  case WM_VSCROLL:
    break;
  case WM_LBUTTONDOWN:
    pWindow->pressButton(wParam, lParam);
    break;
  case WM_LBUTTONUP:
    pWindow->releaseButton(wParam, lParam);
    break;
  case WM_CREATE:
    pWindow->create();
    break;
  case WM_KILLFOCUS:
    pWindow->loseFocus(wParam, lParam);
    break;
  case WM_PAINT:
    pWindow->paint();
    return 0;
  case WM_CTLCOLORSTATIC:
    // for static controls we pass the hwnd of the window itself
    return pWindow->controlColor(wParam, lParam);
  case WM_COMMAND:
    pWindow->command(wParam, lParam);
    break;
  case WM_DEVICECHANGE:
    if (!pWindow->m_deviceCallback) {
      break;
    }
    // Output some messages to the window.
    if (wParam == DBT_DEVICEARRIVAL) {
      pWindow->m_deviceCallback(pWindow->m_callbackArg, (DEV_BROADCAST_HDR *)lParam, true);
    } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
      pWindow->m_deviceCallback(pWindow->m_callbackArg, (DEV_BROADCAST_HDR *)lParam, false);
    }
    // should we handle DBT_DEVNODES_CHANGED ?
    break;
  case WM_CLOSE:
    pWindow->setVisible(false);
    return 0;
  case WM_QUIT:
  case WM_DESTROY:
    return 0;
  default:
    break;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void VChildWindow::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VChildWindow::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VChildWindow::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.lpszClassName = WC_VCHILDWINDOW;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  //m_wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
  RegisterClass(&m_wc);
}