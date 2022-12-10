#include "VWindow.h"

// Vortex Engine includes
#include "EditorConfig.h"

// Editor includes
#include "VortexEditor.h"

#include "resource.h"

using namespace std;

WNDCLASS VWindow::m_wc = {0};

VWindow::VWindow() :
  m_hwnd(nullptr),
  m_children(),
  m_pParent(nullptr),
  m_callbackArg(nullptr)
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
  m_hwnd = CreateWindow(EDITOR_CLASS, title.c_str(),
    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
    (desktop.right / 2) - (width / 2), (desktop.bottom / 2) - (height / 2),
    840, 680, nullptr, nullptr, hInstance, nullptr);
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
  VWindow *child = getChild((HMENU)menuID);
  if (!child){ 
    return;
  }
  child->command(wParam, lParam);
}

void VWindow::pressButton()
{
}

void VWindow::releaseButton()
{
}

uint32_t VWindow::addChild(HMENU menuID, VWindow *child)
{
  child->m_pParent = this;
  child->m_callbackArg = m_callbackArg;
  m_children.insert(make_pair(menuID, child));
  return (uint32_t)(m_children.size() - 1);
}

VWindow *VWindow::getChild(HMENU id)
{
  auto result = m_children.find(id);
  if (result == m_children.end()) {
    return nullptr;
  }
  return result->second;
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
  m_wc.lpszClassName = EDITOR_CLASS;
  m_wc.hbrBackground = CreateSolidBrush(backcol);
  m_wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
  RegisterClass(&m_wc);
}
