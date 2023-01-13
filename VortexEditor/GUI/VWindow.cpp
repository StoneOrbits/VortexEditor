#include "VWindow.h"

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

#include <CommCtrl.h>
#include <Dbt.h>

#include "resource.h"

using namespace std;

WNDCLASS VWindow::m_wc = {0};

// This GUID is for all USB serial host PnP drivers
GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72,
                    0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };

// The window class
#define VWINDOW      "VWINDOW"

VWindow::VWindow() :
  m_hwnd(nullptr),
  m_children(),
  m_pParent(nullptr),
  m_callbackArg(nullptr),
  m_hDeviceNotify(nullptr),
  m_deviceCallback(nullptr)
{
}

VWindow::VWindow(HINSTANCE hinstance, const string &title,
  COLORREF backcol, uint32_t width, uint32_t height,
  void *callbackArg) :
  VWindow()
{
  init(hinstance, title, backcol, width, height, callbackArg);
}

VWindow::~VWindow()
{
  cleanup();
}

void VWindow::init(HINSTANCE hInstance, const string &title,
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
  m_hwnd = CreateWindow(VWINDOW, title.c_str(),
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

void VWindow::cleanup()
{
}

bool VWindow::process(MSG &msg)
{
  return IsDialogMessage(m_hwnd, &msg);
}

void VWindow::create()
{
}

void VWindow::paint()
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(m_hwnd, &ps);

  EndPaint(m_hwnd, &ps);
}

void VWindow::command(WPARAM wParam, LPARAM lParam)
{
  uintptr_t menuID = LOWORD(wParam);
  if (menuID == ID_FILE_QUIT) {
    PostQuitMessage(0);
    return;
  }
  VWindow *child = getChild(menuID);
  if (child){
    child->command(wParam, lParam);
    return;
  }
  VMenuCallback menuCallback = getCallback(menuID);
  if (menuCallback) {
    menuCallback(m_callbackArg, menuID);
    return;
  }
}

void VWindow::pressButton()
{
}

void VWindow::releaseButton()
{
}

uint32_t VWindow::addChild(uintptr_t menuID, VWindow *child)
{
  child->m_pParent = this;
  child->m_callbackArg = m_callbackArg;
  m_children.insert(make_pair(menuID, child));
  return (uint32_t)(m_children.size() - 1);
}

VWindow *VWindow::getChild(uintptr_t id)
{
  auto result = m_children.find(id);
  if (result == m_children.end()) {
    return nullptr;
  }
  return result->second;
}

uint32_t VWindow::addCallback(uintptr_t menuID, VMenuCallback callback)
{
  m_menuCallbacks.insert(make_pair(menuID, callback));
  return (uint32_t)m_menuCallbacks.size();
}

VWindow::VMenuCallback VWindow::getCallback(uintptr_t menuID)
{
  auto entry = m_menuCallbacks.find(menuID);
  if (entry == m_menuCallbacks.end()) {
    return nullptr;
  }
  return entry->second;
}

void VWindow::installDeviceCallback(VDeviceCallback callback)
{
  // only one is allowed
  if (m_deviceCallback || m_hDeviceNotify) {
    return;
  }
  m_deviceCallback = callback;
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
  ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
  NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_classguid = WceusbshGUID;

  m_hDeviceNotify = RegisterDeviceNotification(m_hwnd,
    &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
}

void VWindow::setTooltip(string text)
{
  if (m_tooltipHwnd) {
    DestroyWindow(m_tooltipHwnd);
    m_tooltipHwnd = nullptr;
  }
  // Create the tooltip. g_hInst is the global instance handle.
  m_tooltipHwnd = CreateWindow(TOOLTIPS_CLASS, NULL,
    WS_POPUP | TTS_ALWAYSTIP,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    m_hwnd, NULL, g_pEditor->hInst(), NULL);
  if (!m_tooltipHwnd) {
    return;
  }

  // Associate the tooltip with the tool.
  TOOLINFO toolInfo = { 0 };
  toolInfo.cbSize = sizeof(toolInfo);
  toolInfo.hwnd = m_hwnd;
  toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  toolInfo.uId = (UINT_PTR)m_hwnd;
  toolInfo.lpszText = (LPSTR)text.c_str();
  SendMessage(m_tooltipHwnd, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

void VWindow::setVisible(bool visible)
{
  ShowWindow(m_hwnd, visible);
}

void VWindow::setEnabled(bool enable)
{
  EnableWindow(m_hwnd, enable);
}

bool VWindow::isVisible() const
{
  return IsWindowVisible(m_hwnd);
}

bool VWindow::isEnabled() const
{
  return IsWindowEnabled(m_hwnd);
}

LRESULT CALLBACK VWindow::window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  VWindow *pWindow = (VWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (!pWindow) {
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  switch (uMsg) {
  case WM_VSCROLL:
    break;
  case WM_LBUTTONDOWN:
    pWindow->pressButton();
    break;
  case WM_LBUTTONUP:
    pWindow->releaseButton();
    break;
  case WM_CTLCOLORSTATIC:
    SetTextColor((HDC)wParam, RGB(0xD0, 0xD0, 0xD0));
    SetBkColor((HDC)wParam, BACK_COL);
    return (INT_PTR)pWindow->m_wc.hbrBackground;
  case WM_CREATE:
    pWindow->create();
    break;
  case WM_PAINT:
    pWindow->paint();
    return 0;
  //case WM_LBUTTONDOWN:
    //g_pEditor->handleWindowClick(LOWORD(lParam), HIWORD(lParam));
    //break;
  case WM_COMMAND:
    pWindow->command(wParam, lParam);
    break;
  case WM_DEVICECHANGE:
    // Output some messages to the window.
    if (wParam == DBT_DEVICEARRIVAL) {
      pWindow->m_deviceCallback(pWindow->m_callbackArg, (DEV_BROADCAST_HDR *)lParam, true);
    } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
      pWindow->m_deviceCallback(pWindow->m_callbackArg, (DEV_BROADCAST_HDR *)lParam, false);
    }
    // should we handle DBT_DEVNODES_CHANGED ?
    break;
  case WM_DESTROY:
    pWindow->cleanup();
    // TODO: proper cleanup
    PostQuitMessage(0);
    break;
  default:
    break;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void VWindow::registerWindowClass(HINSTANCE hInstance, COLORREF backcol)
{
  if (m_wc.lpfnWndProc == VWindow::window_proc) {
    // alredy registered
    return;
  }
  // class registration
  m_wc.lpfnWndProc = VWindow::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.lpszClassName = VWINDOW;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
  RegisterClass(&m_wc);
}
