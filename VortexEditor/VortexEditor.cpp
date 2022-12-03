#include "VortexEditor.h"

#include "VortexEngine/VortexEngine/src/VortexConfig.h"

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
m_hInstance(NULL),
  m_bkbrush(NULL),
  m_wc(),
  m_hwnd(NULL)
{
}

VortexEditor::~VortexEditor()
{
}

bool VortexEditor::init(HINSTANCE hInstance)
{
  if (g_pEditor) {
    return false;
  }
  g_pEditor = this;

  m_bkbrush = CreateSolidBrush(bkcolor);

  // class registration
  memset(&m_wc, 0, sizeof(m_wc));
  m_wc.lpfnWndProc = VortexEditor::window_proc;
  m_wc.hInstance = hInstance;
  m_wc.lpszClassName = L"Vortex Editor";
  m_wc.hbrBackground = m_bkbrush;
  RegisterClass(&m_wc);

  // get desktop rect so we can center the window
  RECT desktop;
  GetClientRect(GetDesktopWindow(), &desktop);

  // create the window
  m_hwnd = CreateWindow(m_wc.lpszClassName, L"Vortex Editor " VORTEX_VERSION,
    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
    (desktop.right / 2) - 240, (desktop.bottom / 2) - 84,
    420, 340, nullptr, nullptr, hInstance, nullptr);
  if (!m_hwnd) {
    MessageBox(nullptr, L"Failed to open window", L"Error", 0);
    return 0;
  }
  return true;
}

void VortexEditor::run()
{
  // main message loop
  MSG msg;
  ShowWindow(m_hwnd, SW_NORMAL);
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!IsDialogMessage(m_hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

LRESULT CALLBACK VortexEditor::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
  case WM_VSCROLL:
    break;
  case WM_CTLCOLORSTATIC:
     return (INT_PTR)g_pEditor->m_bkbrush;
  case WM_CREATE:
    //g_pEditor->create(hwnd);
    break;
  case WM_PAINT:
    //g_pEditor->paint(hwnd);
    return 0;
  case WM_LBUTTONDOWN:
    //g_pEditor->handleWindowClick(LOWORD(lParam), HIWORD(lParam));
    break;
  case WM_COMMAND:
    //g_pEditor->command(wParam, lParam);
    break;
  case WM_DESTROY:
    //g_pTestFramework->cleanup();
    PostQuitMessage(0);
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
