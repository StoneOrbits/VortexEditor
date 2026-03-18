#include "VBehaviourNode.h"
#include "VortexEngine.h"
#include "Behaviours/BehaviourNode.h"
#include "VortexBehaviourEditor.h"

#include "VortexEditor.h"

#include <windows.h>
#include <stdio.h>

#define NODE_W 150
#define NODE_H 80
#define SOCKET_R 6

#define PARAM_A_MENU_ID 50998
#define PARAM_B_MENU_ID 50999

#pragma optimize("", off)

VBehaviourNode::VBehaviourNode() :
  VWindow(),
  m_engine(nullptr),
  m_node(nullptr),
  m_param0(),
  m_param1(),
  m_selected(false),
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

  // create text boxes
  char buf[32];
  sprintf_s(buf, "%.3f", node->param1);
  sprintf_s(buf, "%.3f", node->param2);
  m_param0.init(inst, *this, buf, RGB(80, 80, 80), 60, 18, 30, 22, PARAM_A_MENU_ID, ParamChangedCallback);
  m_param1.init(inst, *this, buf, RGB(80, 80, 80), 60, 18, 30, 42, PARAM_B_MENU_ID, ParamChangedCallback);
  m_param0.setCallbackArg(this);
  m_param1.setCallbackArg(this);
  m_param0.setEnabled(true);
  m_param1.setEnabled(true);
}

void VBehaviourNode::ParamChangedCallback(void *arg, VWindow *window)
{
  VBehaviourNode *node = (VBehaviourNode *)arg;
  if (!node) {
    // error
    return;
  }
  node->updateParams();
}

void VBehaviourNode::cleanup()
{
  m_param0.cleanup();
  m_param1.cleanup();
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

    RECT r0, r1;
    GetWindowRect(m_param0.hwnd(), &r0);
    GetWindowRect(m_param1.hwnd(), &r1);
    POINT screenPt = pt;
    ClientToScreen(m_hwnd, &screenPt);

    if (PtInRect(&r0, screenPt) || PtInRect(&r1, screenPt)) {
        return;
    }


    m_dragging = true;
    m_dragOffsetX = pt.x;
    m_dragOffsetY = pt.y;
    SetCapture(m_hwnd);

    SetFocus(m_hwnd);
}

void VBehaviourNode::releaseButton(WPARAM wParam, LPARAM lParam)
{
  if (!m_dragging) return;
  m_dragging = false;
  ReleaseCapture();

  HWND parent = GetParent(m_hwnd);
  VortexBehaviourEditor *editor =
    (VortexBehaviourEditor *)GetWindowLongPtr(parent, GWLP_USERDATA);
  if (editor) {
    editor->redraw();
  }
}

void VBehaviourNode::rightClick(WPARAM wParam, LPARAM lParam)
{
  POINT pt = { LOWORD(lParam), HIWORD(lParam) };
  POINT screenPt = pt;
  ClientToScreen(m_hwnd, &screenPt);

  showContextMenu(
    screenPt.x,
    screenPt.y,
    pt.x,
    pt.y
  );
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
  // notifications are now handled by VTextBox callbacks
}

void VBehaviourNode::updateParams()
{
  m_node->param1 = m_param0.getFloatValue();
  m_node->param2 = m_param1.getFloatValue();
}

BehaviourNode *VBehaviourNode::node() { return m_node; }

int VBehaviourNode::outputSocketX() { RECT r; GetWindowRect(m_hwnd, &r); return r.right; }
int VBehaviourNode::outputSocketY() { RECT r; GetWindowRect(m_hwnd, &r); return r.top + NODE_H / 2; }
int VBehaviourNode::inputSocketX(int i) { RECT r; GetWindowRect(m_hwnd, &r); return r.left; }
int VBehaviourNode::inputSocketY(int i) { RECT r; GetWindowRect(m_hwnd, &r); return r.top + NODE_H / 2 + (i * 16); }

const char *VBehaviourNode::nodeTypeName(Behaviours::NodeType t)
{
  switch (t) {
  case Behaviours::NODE_INPUT_TIME: return "Time";
  case Behaviours::NODE_INPUT_DELTA_TIME: return "Delta Time";
  case Behaviours::NODE_INPUT_RANDOM: return "Random";
  case Behaviours::NODE_INPUT_CONSTANT: return "Constant";

  case Behaviours::NODE_INPUT_ACCEL_MOTION: return "Accel Motion";
  case Behaviours::NODE_INPUT_ACCEL_NORMALIZED: return "Accel Norm";
  case Behaviours::NODE_INPUT_ACCEL_CURVED: return "Accel Curve";
  case Behaviours::NODE_INPUT_ACCEL_FILTERED: return "Accel Filter";
  case Behaviours::NODE_INPUT_ACCEL_DIR_X: return "Accel X";
  case Behaviours::NODE_INPUT_ACCEL_DIR_Y: return "Accel Y";
  case Behaviours::NODE_INPUT_ACCEL_DIR_Z: return "Accel Z";
  case Behaviours::NODE_INPUT_ACCEL_PITCH: return "Pitch";
  case Behaviours::NODE_INPUT_ACCEL_ROLL: return "Roll";
  case Behaviours::NODE_INPUT_ACCEL_TILT: return "Tilt";

  case Behaviours::NODE_MODIFIER_ABS: return "Abs";
  case Behaviours::NODE_MODIFIER_ADD: return "Add";
  case Behaviours::NODE_MODIFIER_SUBTRACT: return "Subtract";
  case Behaviours::NODE_MODIFIER_MULTIPLY: return "Multiply";
  case Behaviours::NODE_MODIFIER_DIVIDE: return "Divide";
  case Behaviours::NODE_MODIFIER_MIN: return "Min";
  case Behaviours::NODE_MODIFIER_MAX: return "Max";
  case Behaviours::NODE_MODIFIER_CLAMP: return "Clamp";
  case Behaviours::NODE_MODIFIER_REMAP: return "Remap";
  case Behaviours::NODE_MODIFIER_CURVE: return "Curve";
  case Behaviours::NODE_MODIFIER_SMOOTHSTEP: return "Smoothstep";
  case Behaviours::NODE_MODIFIER_SIN: return "Sin";
  case Behaviours::NODE_MODIFIER_COS: return "Cos";
  case Behaviours::NODE_MODIFIER_THRESHOLD: return "Threshold";
  case Behaviours::NODE_MODIFIER_GREATER: return "Greater";
  case Behaviours::NODE_MODIFIER_LESS: return "Less";
  case Behaviours::NODE_MODIFIER_LERP: return "Lerp";
  case Behaviours::NODE_MODIFIER_SELECT: return "Select";

  case Behaviours::NODE_FUNCTIONAL_MODE_BLEND: return "Mode Blend";
  case Behaviours::NODE_FUNCTIONAL_MODE_ADD: return "Mode Add";
  case Behaviours::NODE_FUNCTIONAL_BRIGHTNESS_SHIFT: return "Brightness";
  case Behaviours::NODE_FUNCTIONAL_COLOR_SHIFT: return "Color Shift";
  case Behaviours::NODE_FUNCTIONAL_PATTERN_SHIFT: return "Pattern Shift";

  default:
    return "Node";
  }
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
  case WM_PAINT:
    if (node) node->paint();
    return 0;

  case WM_LBUTTONDOWN:
    if (node) node->pressButton(wParam, lParam);
    return 0;

  case WM_LBUTTONUP:
    if (node) node->releaseButton(wParam, lParam);
    return 0;

  case WM_RBUTTONUP:
    if (node) node->rightClick(wParam, lParam);
    return 0;

  case WM_MOUSEMOVE:
    if (node) node->mouseMove(wParam, lParam);
    return 0;

  case WM_COMMAND:
    if (lParam) {
      VTextBox *textbox = (VTextBox *)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
      if (textbox) {
        textbox->command(wParam, lParam);
        return 0;
      }
    }
    break;

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

  case WM_DESTROY:
    return 0;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void VBehaviourNode::select()
{
  m_selected = true;
  redraw();
}

void VBehaviourNode::deselect()
{
  m_selected = false;

  // Remove focus from textboxes
  if (m_param0.hwnd()) {
    SendMessage(m_param0.hwnd(), WM_KILLFOCUS, 0, 0);
  }
  if (m_param1.hwnd()) {
    SendMessage(m_param1.hwnd(), WM_KILLFOCUS, 0, 0);
  }

  // Optionally redraw to show visual deselection
  redraw();
}

void VBehaviourNode::showContextMenu(int screenX, int screenY, int clientX, int clientY)
{
  HMENU menu = CreatePopupMenu();
  AppendMenu(menu, MF_STRING, 9999, "Delete Node");
  int cmd = TrackPopupMenu(menu, TPM_RETURNCMD, screenX, screenY, 0, hwnd(), NULL);
  DestroyMenu(menu);
  if (cmd == 0) return;
  if (cmd == 9999) {
    g_pEditor->behaviourEditor().deleteNode(this);
    return;
  }
  redraw();
}
