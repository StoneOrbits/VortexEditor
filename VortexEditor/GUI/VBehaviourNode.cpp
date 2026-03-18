#include "VBehaviourNode.h"
#include "VortexEngine.h"
#include "Behaviours/BehaviourNode.h"
#include "VortexBehaviourEditor.h"

#include <windows.h>
#include <stdio.h>

#define NODE_W 150
#define NODE_H 80
#define SOCKET_R 6

VBehaviourNode::VBehaviourNode() :
  VWindow(),
  m_engine(nullptr),
  m_node(nullptr),
  m_param0(nullptr),
  m_param1(nullptr),
  m_dragging(false),
  m_dragOffsetX(0),
  m_dragOffsetY(0)
{
}

VBehaviourNode::~VBehaviourNode()
{
  cleanup();
}

void VBehaviourNode::init(HINSTANCE inst, HWND parent, VortexEngine &engine, BehaviourNode *node, int x, int y)
{
  m_engine = &engine;
  m_node = node;

  static int nodeClassRegistered = 0;
  if (!nodeClassRegistered) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = NodeWndProc;
    wc.hInstance = inst;
    wc.lpszClassName = "VBehaviourNodeClass";
    wc.hbrBackground = NULL;
    RegisterClass(&wc);
    nodeClassRegistered = 1;
  }

  m_hwnd = CreateWindowEx(
    0,
    "VBehaviourNodeClass",
    "",
    WS_CHILD | WS_VISIBLE,
    x, y,
    NODE_W, NODE_H,
    parent,
    nullptr,
    inst,
    nullptr
  );

  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  char buf[32];
  sprintf_s(buf, "%.3f", node->param1);
  m_param0 = CreateWindowEx(0, "EDIT", buf, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
    30, 22, 60, 18, m_hwnd, (HMENU)1, inst, nullptr);

  sprintf_s(buf, "%.3f", node->param2);
  m_param1 = CreateWindowEx(0, "EDIT", buf, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
    30, 42, 60, 18, m_hwnd, (HMENU)2, inst, nullptr);
}

void VBehaviourNode::cleanup()
{
  if (m_param0) DestroyWindow(m_param0);
  if (m_param1) DestroyWindow(m_param1);
  if (m_hwnd) DestroyWindow(m_hwnd);
}

void VBehaviourNode::create() {}

void VBehaviourNode::paint()
{
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(m_hwnd, &ps);

  RECT rc;
  GetClientRect(m_hwnd, &rc);

  HBRUSH bg = CreateSolidBrush(nodeColor());
  FillRect(dc, &rc, bg);
  DeleteObject(bg);

  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(220, 220, 220));
  TextOutA(dc, 6, 4, nodeTypeName(m_node->type()), strlen(nodeTypeName(m_node->type())));

  uint8_t inputs = m_node->inputCount();
  for (uint8_t i = 0; i < inputs; i++) {
    int sx = -SOCKET_R;
    int sy = NODE_H / 2 - SOCKET_R + (i * 16);
    Ellipse(dc, sx, sy, sx + SOCKET_R * 2, sy + SOCKET_R * 2);
  }

  Ellipse(dc,
    NODE_W - SOCKET_R,
    NODE_H / 2 - SOCKET_R,
    NODE_W + SOCKET_R,
    NODE_H / 2 + SOCKET_R
  );

  EndPaint(m_hwnd, &ps);
}

void VBehaviourNode::pressButton(WPARAM wParam, LPARAM lParam)
{
  POINT pt = { LOWORD(lParam), HIWORD(lParam) };
  m_dragging = true;
  m_dragOffsetX = pt.x;
  m_dragOffsetY = pt.y;
  SetCapture(m_hwnd);
}

void VBehaviourNode::releaseButton(WPARAM wParam, LPARAM lParam)
{
  m_dragging = false;
  ReleaseCapture();

  HWND parent = GetParent(m_hwnd);
  VortexBehaviourEditor *editor =
    (VortexBehaviourEditor *)GetWindowLongPtr(parent, GWLP_USERDATA);
  if (editor) {
    editor->redraw();
  }
}

void VBehaviourNode::mouseMove(WPARAM wParam, LPARAM lParam)
{
  if (!m_dragging) return;

  POINT pt;
  GetCursorPos(&pt);

  HWND parent = GetParent(m_hwnd);
  ScreenToClient(parent, &pt);

  int x = pt.x - m_dragOffsetX;
  int y = pt.y - m_dragOffsetY;

  SetWindowPos(
    m_hwnd,
    nullptr,
    x,
    y,
    0,
    0,
    SWP_NOZORDER | SWP_NOSIZE
  );
}

void VBehaviourNode::command(WPARAM wParam, LPARAM lParam)
{
  if (HIWORD(wParam) != EN_KILLFOCUS) return;
  updateParams();
}

void VBehaviourNode::updateParams()
{
  char buf[64];
  GetWindowTextA(m_param0, buf, sizeof(buf));
  m_node->param1 = (float)atof(buf);

  GetWindowTextA(m_param1, buf, sizeof(buf));
  m_node->param2 = (float)atof(buf);
}

BehaviourNode *VBehaviourNode::node() { return m_node; }

int VBehaviourNode::outputSocketX() { RECT r; GetWindowRect(m_hwnd, &r); return r.right; }
int VBehaviourNode::outputSocketY() { RECT r; GetWindowRect(m_hwnd, &r); return r.top + NODE_H / 2; }
int VBehaviourNode::inputSocketX(int i) { RECT r; GetWindowRect(m_hwnd, &r); return r.left; }
int VBehaviourNode::inputSocketY(int i) { RECT r; GetWindowRect(m_hwnd, &r); return r.top + NODE_H / 2 + (i * 16); }

const char *VBehaviourNode::nodeTypeName(Behaviours::NodeType t)
{
  switch (t) {
  case Behaviours::NODE_ACCEL_MOTION: return "Accel Motion";
  case Behaviours::NODE_ACCEL_NORMALIZED: return "Accel Norm";
  case Behaviours::NODE_ACCEL_CURVED: return "Accel Curve";
  case Behaviours::NODE_ACCEL_FILTERED: return "Accel Filter";
  case Behaviours::NODE_ACCEL_DIR_X: return "Accel X";
  case Behaviours::NODE_ACCEL_DIR_Y: return "Accel Y";
  case Behaviours::NODE_ACCEL_DIR_Z: return "Accel Z";
  case Behaviours::NODE_ACCEL_TILT: return "Tilt";
  case Behaviours::NODE_ABS: return "Abs";
  case Behaviours::NODE_ADD: return "Add";
  case Behaviours::NODE_MULTIPLY: return "Multiply";
  case Behaviours::NODE_CLAMP: return "Clamp";
  case Behaviours::NODE_CURVE: return "Curve";
  case Behaviours::NODE_THRESHOLD: return "Threshold";
  case Behaviours::NODE_MODE_BLEND: return "Blend";
  }
  return "Node";
}

COLORREF VBehaviourNode::nodeColor()
{
  switch (m_engine->behaviours().getSubtype(m_node->type())) {
  case Behaviours::SUBTYPE_INPUT: return RGB(60, 120, 220);
  case Behaviours::SUBTYPE_MODIFIER: return RGB(220, 120, 60);
  case Behaviours::SUBTYPE_FUNCTIONAL: return RGB(100, 200, 100);
  default: return RGB(80, 80, 80);
  }
}

INT_PTR VBehaviourNode::controlColor(WPARAM wParam, LPARAM lParam)
{
  HDC hdc = (HDC)wParam;
  SetTextColor(hdc, RGB(220, 220, 220));
  SetBkMode(hdc, TRANSPARENT);
  static HBRUSH br = nullptr;
  if (br) DeleteObject(br);
  br = CreateSolidBrush(nodeColor());
  return (INT_PTR)br;
}

LRESULT CALLBACK VBehaviourNode::NodeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  VBehaviourNode *node = (VBehaviourNode *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (msg) {
  case WM_PAINT: if (node) node->paint(); return 0;
  case WM_LBUTTONDOWN: if (node) node->pressButton(wParam, lParam); return 0;
  case WM_LBUTTONUP: if (node) node->releaseButton(wParam, lParam); return 0;
  case WM_MOUSEMOVE: if (node) node->mouseMove(wParam, lParam); return 0;
  case WM_COMMAND: if (node) node->command(wParam, lParam); return 0;
  case WM_CTLCOLORSTATIC:
    if (node) {
      HDC hdc = (HDC)wParam;
      SetTextColor(hdc, RGB(220, 220, 220));
      SetBkMode(hdc, TRANSPARENT);
      static HBRUSH br = nullptr;
      if (br) DeleteObject(br);
      br = CreateSolidBrush(node->nodeColor());
      return (INT_PTR)br;
    }
    break;
  case WM_DESTROY: return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}