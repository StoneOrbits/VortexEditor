#include "VortexBehaviourEditor.h"
#include "GUI/VBehaviourNode.h"
#include "GUI/VWindow.h"

#include "Behaviours/BehaviourNode.h"
#include "VortexEngine.h"
#include "EditorConfig.h"

#include "resource.h"

#include <windows.h>
#include <stdio.h>

#define NODE_W 150
#define NODE_H 80
#define SOCKET_R 6

#pragma optimize("", off)

VortexBehaviourEditor::VortexBehaviourEditor(VortexEngine &engine) :
  m_engine(engine),
  m_behaviourLinkOverlay(),
  m_inst(nullptr),
  m_isOpen(false),
  m_hIcon(NULL),
  m_nodes{},
  m_nodeCount(0),
  m_linking(false),
  m_linkNode(nullptr),
  m_linkMouseX(0),
  m_linkMouseY(0),
  m_dragNode(nullptr),
  m_dragOffsetX(0),
  m_dragOffsetY(0)
{
}

VortexBehaviourEditor::~VortexBehaviourEditor()
{
  for (uint32_t i = 0; i < m_nodeCount; i++)
    delete m_nodes[i];
}

bool VortexBehaviourEditor::init(HINSTANCE inst)
{
  m_inst = inst;

  VChildWindow::init(
    inst,
    "Vortex Behaviour Editor",
    BACK_COL,
    900,
    600,
    this
  );

  setVisible(false);
  setEnabled(true);

  m_hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
}

void VortexBehaviourEditor::show()
{
  if (m_isOpen)
    return;

  populateFromBehaviours();

  setVisible(true);
  m_isOpen = true;
}

void VortexBehaviourEditor::hide()
{
  m_isOpen = false;
  ShowWindow(hwnd(), SW_HIDE);
}

void VortexBehaviourEditor::run()
{
}

void VortexBehaviourEditor::redraw()
{
  InvalidateRect(hwnd(), NULL, FALSE);
}

void VortexBehaviourEditor::populateFromBehaviours()
{
  for (uint32_t i = 0; i < m_nodeCount; i++)
    delete m_nodes[i];
  m_nodeCount = 0;

  uint8_t total = m_engine.behaviours().nodeCount();

  for (uint8_t i = 0; i < total && m_nodeCount < MAX_BEHAVIOUR_NODES; i++) {
    BehaviourNode *bn = m_engine.behaviours().node(i);
    if (!bn)
      continue;

    int column = 1;
    switch (m_engine.behaviours().getSubtype(bn->type())) {
    case Behaviours::SUBTYPE_INPUT: column = 0; break;
    case Behaviours::SUBTYPE_MODIFIER: column = 1; break;
    case Behaviours::SUBTYPE_FUNCTIONAL: column = 2; break;
    default: column = 1; break;
    }

    int columnCount = 0;
    for (uint8_t j = 0; j < i; j++) {
      BehaviourNode *other = m_engine.behaviours().node(j);
      if (m_engine.behaviours().getSubtype(other->type()) ==
        m_engine.behaviours().getSubtype(bn->type()))
        columnCount++;
    }

    int x = 50 + column * 200;
    int y = 50 + columnCount * 100;

    VBehaviourNode *node = new VBehaviourNode();
    node->init(
      m_inst,
      hwnd(),
      m_engine,
      bn,
      x,
      y
    );

    m_nodes[m_nodeCount++] = node;
  }

  redraw();
}

VBehaviourNode *VortexBehaviourEditor::findNodeFromBehaviour(BehaviourNode *bn)
{
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    if (m_nodes[i]->node() == bn)
      return m_nodes[i];
  }
  return nullptr;
}

void VortexBehaviourEditor::drawLinks(HDC dc)
{
  HPEN pen = CreatePen(PS_SOLID, 2, RGB(200, 200, 200));
  HPEN old = (HPEN)SelectObject(dc, pen);

  for (uint32_t i = 0; i < m_nodeCount; i++) {
    BehaviourNode *dst = m_nodes[i]->node();

    for (uint8_t j = 0; j < dst->inputCount(); j++) {
      BehaviourNode *src = dst->input(j);
      if (!src)
        continue;

      VBehaviourNode *srcNode = findNodeFromBehaviour(src);
      if (!srcNode)
        continue;

      POINT p1 = { srcNode->outputSocketX(), srcNode->outputSocketY() };
      POINT p2 = { m_nodes[i]->inputSocketX(j), m_nodes[i]->inputSocketY(j) };

      ScreenToClient(hwnd(), &p1);
      ScreenToClient(hwnd(), &p2);

      MoveToEx(dc, p1.x, p1.y, NULL);
      LineTo(dc, p2.x, p2.y);
    }
  }

  if (m_linking && m_linkNode) {
    POINT p = { m_linkNode->outputSocketX(), m_linkNode->outputSocketY() };
    ScreenToClient(hwnd(), &p);

    MoveToEx(dc, p.x, p.y, NULL);
    LineTo(dc, m_linkMouseX, m_linkMouseY);
  }

  SelectObject(dc, old);
  DeleteObject(pen);
}

void VortexBehaviourEditor::showContextMenu(int screenX, int screenY, int clientX, int clientY)
{
  HMENU menu = CreatePopupMenu();

  struct MenuItem
  {
    uint32_t id;
    Behaviours::NodeType type;
    const char *name;
  };

  MenuItem items[] =
  {
      {1,m_engine.behaviours().NODE_ACCEL_MOTION,"Accel Motion"},
      {2,m_engine.behaviours().NODE_ACCEL_NORMALIZED,"Accel Normalized"},
      {3,m_engine.behaviours().NODE_ACCEL_CURVED,"Accel Curved"},
      {4,m_engine.behaviours().NODE_ACCEL_FILTERED,"Accel Filtered"},
      {5,m_engine.behaviours().NODE_ACCEL_DIR_X,"Accel Dir X"},
      {6,m_engine.behaviours().NODE_ACCEL_DIR_Y,"Accel Dir Y"},
      {7,m_engine.behaviours().NODE_ACCEL_DIR_Z,"Accel Dir Z"},
      {8,m_engine.behaviours().NODE_ACCEL_TILT,"Accel Tilt"},
      {9,m_engine.behaviours().NODE_ABS,"Abs"},
      {10,m_engine.behaviours().NODE_ADD,"Add"},
      {11,m_engine.behaviours().NODE_MULTIPLY,"Multiply"},
      {12,m_engine.behaviours().NODE_CLAMP,"Clamp"},
      {13,m_engine.behaviours().NODE_CURVE,"Curve"},
      {14,m_engine.behaviours().NODE_THRESHOLD,"Threshold"},
      {15,m_engine.behaviours().NODE_MODE_BLEND,"Mode Blend"}
  };

  for (auto &i : items)
    AppendMenu(menu, MF_STRING, i.id, i.name);

  int cmd = TrackPopupMenu(menu, TPM_RETURNCMD, screenX, screenY, 0, hwnd(), NULL);
  DestroyMenu(menu);

  if (cmd == 0) return;

  Behaviours::NodeType type = m_engine.behaviours().NODE_ADD;
  for (auto &i : items)
    if (i.id == (uint32_t)cmd)
      type = i.type;

  uint8_t idx = m_engine.behaviours().create(type);
  if (idx == 255) return;

  BehaviourNode *bn = m_engine.behaviours().node(idx);
  VBehaviourNode *node = new VBehaviourNode();
  node->init(
    m_inst,
    hwnd(),
    m_engine,
    bn,
    clientX,
    clientY
  );

  if (m_nodeCount < MAX_BEHAVIOUR_NODES)
    m_nodes[m_nodeCount++] = node;

  redraw();
}

void VortexBehaviourEditor::paint()
{
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(hwnd(), &ps);

  RECT rc;
  GetClientRect(hwnd(), &rc);

  HBRUSH bg = CreateSolidBrush(RGB(35, 35, 35));
  FillRect(dc, &rc, bg);
  DeleteObject(bg);

  drawLinks(dc);

  EndPaint(hwnd(), &ps);
}

void VortexBehaviourEditor::mouseMove(WPARAM wParam, LPARAM lParam)
{
  POINT pt = { LOWORD(lParam), HIWORD(lParam) };

  if (m_linking) {
    m_linkMouseX = pt.x;
    m_linkMouseY = pt.y;
    redraw();
    return;
  }

  if (m_dragNode) {
    int newX = pt.x - m_dragOffsetX;
    int newY = pt.y - m_dragOffsetY;

    SetWindowPos(
      m_dragNode->hwnd(),
      nullptr,
      newX,
      newY,
      0,
      0,
      SWP_NOZORDER | SWP_NOSIZE
    );
    redraw();
  }
}

void VortexBehaviourEditor::pressButton(WPARAM wParam, LPARAM lParam)
{
  POINT pt = { LOWORD(lParam), HIWORD(lParam) };

  SetCapture(hwnd());

  // --- linking start ---
  VBehaviourNode* out = findOutputSocket(pt.x, pt.y);
  if (out) {
    m_linking = true;
    m_linkNode = out;
    m_linkMouseX = pt.x;
    m_linkMouseY = pt.y;
    redraw();
    return;
  }

  // --- drag start ---
  m_dragNode = nullptr;

  for (uint32_t i = 0; i < m_nodeCount; i++) {
    RECT r;
    GetWindowRect(m_nodes[i]->hwnd(), &r);

    POINT tl = { r.left, r.top };
    ScreenToClient(hwnd(), &tl);

    RECT cr = {
      tl.x,
      tl.y,
      tl.x + NODE_W,
      tl.y + NODE_H
    };

    if (PtInRect(&cr, pt)) {
      m_dragNode = m_nodes[i];
      m_dragOffsetX = pt.x - cr.left;
      m_dragOffsetY = pt.y - cr.top;
      break;
    }
  }
}

void VortexBehaviourEditor::releaseButton(WPARAM wParam, LPARAM lParam)
{
  ReleaseCapture();

  POINT pt = { LOWORD(lParam), HIWORD(lParam) };

  if (m_linking && m_linkNode) {

    int socketIndex = -1;
    VBehaviourNode* target = findInputSocket(pt.x, pt.y, socketIndex);

    if (target && target != m_linkNode && socketIndex != -1) {

      BehaviourNode *src = m_linkNode->node();
      BehaviourNode *dst = target->node();

      dst->addInput(src);
    }

    m_linking = false;
    m_linkNode = nullptr;
  }

  m_dragNode = nullptr;

  redraw();
}

void VortexBehaviourEditor::rightClick(WPARAM wParam, LPARAM lParam)
{
  POINT client = { LOWORD(lParam), HIWORD(lParam) };
  POINT screen = client;

  ClientToScreen(hwnd(), &screen);

  showContextMenu(
    screen.x,
    screen.y,
    client.x,
    client.y
  );
}

VBehaviourNode* VortexBehaviourEditor::findOutputSocket(int x, int y)
{
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    int sx = m_nodes[i]->outputSocketX();
    int sy = m_nodes[i]->outputSocketY();

    POINT p = { sx, sy };
    ScreenToClient(hwnd(), &p);

    int dx = x - p.x;
    int dy = y - p.y;

    if (dx * dx + dy * dy <= SOCKET_R * SOCKET_R * 4)
      return m_nodes[i];
  }
  return nullptr;
}

VBehaviourNode* VortexBehaviourEditor::findInputSocket(int x, int y, int &socketIndex)
{
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    BehaviourNode *bn = m_nodes[i]->node();

    for (uint8_t j = 0; j < bn->inputCount(); j++) {

      int sx = m_nodes[i]->inputSocketX(j);
      int sy = m_nodes[i]->inputSocketY(j);

      POINT p = { sx, sy };
      ScreenToClient(hwnd(), &p);

      int dx = x - p.x;
      int dy = y - p.y;

      if (dx * dx + dy * dy <= SOCKET_R * SOCKET_R * 4) {
        socketIndex = j;
        return m_nodes[i];
      }
    }
  }
  return nullptr;
}
