#include "VortexBehaviourEditor.h"
#include "GUI/VBehaviourNode.h"
#include "GUI/VWindow.h"

#include "VortexEditor.h"
#include "VortexPort.h"

#include "Behaviours/BehaviourNode.h"
#include "VortexEngine.h"
#include "EditorConfig.h"

#include "resource.h"

#include <windows.h>
#include <stdio.h>

#define NODE_W 150
#define NODE_H 80
#define SOCKET_R 6

#define BEHAVIOUR_SEND_ID 59901

VortexBehaviourEditor::VortexBehaviourEditor(VortexEngine &engine) :
  m_engine(engine),
  m_inst(nullptr),
  m_button(),
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

  m_button.init(inst, *this, "Send", BACK_COL, 64, 32, 320, 80, BEHAVIOUR_SEND_ID, sendCallback);

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

  VBehaviourNode *hitNode = nullptr;
  bool onTextbox = false;
  hitNode = hitTestNode({ clientX, clientY }, onTextbox);

  if (hitNode) {
    AppendMenu(menu, MF_STRING, 9999, "Delete Node"); // 9999 = delete command
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
  }

  // existing creation items...
  struct MenuItem { uint32_t id; Behaviours::NodeType type; const char *name; int group; };
  MenuItem items[] =
  {
    // ---------------------
    // Input Nodes
    // ---------------------
    {1,  m_engine.behaviours().NODE_INPUT_TIME,               "Time", 0},
    {2,  m_engine.behaviours().NODE_INPUT_DELTA_TIME,         "Delta Time", 0},
    {3,  m_engine.behaviours().NODE_INPUT_RANDOM,             "Random", 0},
    {4,  m_engine.behaviours().NODE_INPUT_CONSTANT,           "Constant", 0},

    {5,  m_engine.behaviours().NODE_INPUT_ACCEL_MOTION,      "Accel Motion", 0},
    {6,  m_engine.behaviours().NODE_INPUT_ACCEL_NORMALIZED,  "Accel Normalized", 0},
    {7,  m_engine.behaviours().NODE_INPUT_ACCEL_CURVED,      "Accel Curved", 0},
    {8,  m_engine.behaviours().NODE_INPUT_ACCEL_FILTERED,    "Accel Filtered", 0},
    {9,  m_engine.behaviours().NODE_INPUT_ACCEL_DIR_X,       "Accel X", 0},
    {10, m_engine.behaviours().NODE_INPUT_ACCEL_DIR_Y,       "Accel Y", 0},
    {11, m_engine.behaviours().NODE_INPUT_ACCEL_DIR_Z,       "Accel Z", 0},
    {12, m_engine.behaviours().NODE_INPUT_ACCEL_PITCH,       "Accel Pitch", 0},
    {13, m_engine.behaviours().NODE_INPUT_ACCEL_ROLL,        "Accel Roll", 0},
    {14, m_engine.behaviours().NODE_INPUT_ACCEL_TILT,        "Accel Tilt", 0},

    // ---------------------
    // Modifier Nodes
    // ---------------------
    {15, m_engine.behaviours().NODE_MODIFIER_ABS,            "Abs", 1},
    {16, m_engine.behaviours().NODE_MODIFIER_ADD,            "Add", 1},
    {17, m_engine.behaviours().NODE_MODIFIER_SUBTRACT,       "Subtract", 1},
    {18, m_engine.behaviours().NODE_MODIFIER_MULTIPLY,       "Multiply", 1},
    {19, m_engine.behaviours().NODE_MODIFIER_DIVIDE,         "Divide", 1},
    {20, m_engine.behaviours().NODE_MODIFIER_MIN,            "Min", 1},
    {21, m_engine.behaviours().NODE_MODIFIER_MAX,            "Max", 1},
    {22, m_engine.behaviours().NODE_MODIFIER_CLAMP,          "Clamp", 1},
    {23, m_engine.behaviours().NODE_MODIFIER_REMAP,          "Remap", 1},
    {24, m_engine.behaviours().NODE_MODIFIER_CURVE,          "Curve", 1},
    {25, m_engine.behaviours().NODE_MODIFIER_SMOOTHSTEP,     "Smoothstep", 1},
    {26, m_engine.behaviours().NODE_MODIFIER_SIN,            "Sin", 1},
    {27, m_engine.behaviours().NODE_MODIFIER_COS,            "Cos", 1},
    {28, m_engine.behaviours().NODE_MODIFIER_THRESHOLD,      "Threshold", 1},
    {29, m_engine.behaviours().NODE_MODIFIER_GREATER,        "Greater", 1},
    {30, m_engine.behaviours().NODE_MODIFIER_LESS,           "Less", 1},
    {31, m_engine.behaviours().NODE_MODIFIER_LERP,           "Lerp", 1},
    {32, m_engine.behaviours().NODE_MODIFIER_SELECT,         "Select", 1},

    // ---------------------
    // Functional Nodes
    // ---------------------
    {33, m_engine.behaviours().NODE_FUNCTIONAL_MODE_BLEND,         "Mode Blend", 2},
    {34, m_engine.behaviours().NODE_FUNCTIONAL_MODE_ADD,           "Mode Add", 2},
    {35, m_engine.behaviours().NODE_FUNCTIONAL_BRIGHTNESS_SHIFT,   "Brightness Shift", 2},
    {36, m_engine.behaviours().NODE_FUNCTIONAL_COLOR_SHIFT,        "Color Shift", 2},
    {37, m_engine.behaviours().NODE_FUNCTIONAL_PATTERN_SHIFT,      "Pattern Shift", 2}
  };


  if (!hitNode) {
    int lastGroup = -1;
    for (auto &i : items) {
      if (i.group != lastGroup && lastGroup != -1)
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
      AppendMenu(menu, MF_STRING, i.id, i.name);
      lastGroup = i.group;
    }
  }

  int cmd = TrackPopupMenu(menu, TPM_RETURNCMD, screenX, screenY, 0, hwnd(), NULL);
  DestroyMenu(menu);

  if (cmd == 0) return;

  if (cmd == 9999 && hitNode) {
    deleteNode(hitNode);
    return;
  }

  // existing creation logic
  Behaviours::NodeType type = m_engine.behaviours().NODE_MODIFIER_ADD;
  for (auto &i : items)
    if (i.id == (uint32_t)cmd)
      type = i.type;

  uint8_t idx = m_engine.behaviours().create(type);
  if (idx == 255) return;

  BehaviourNode *bn = m_engine.behaviours().node(idx);
  VBehaviourNode *node = new VBehaviourNode();
  node->init(m_inst, hwnd(), m_engine, bn, clientX, clientY);

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

// Deselect all nodes
void VortexBehaviourEditor::deselectAllNodes()
{
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    m_nodes[i]->deselect();
  }
  // restore cursor to normal
  SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

// Select a single node, deselect others
void VortexBehaviourEditor::selectNode(VBehaviourNode* node)
{
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    if (m_nodes[i] == node) m_nodes[i]->select();
    else m_nodes[i]->deselect();
  }
}

// Check if a point is inside a node (returns node pointer or nullptr)
VBehaviourNode* VortexBehaviourEditor::hitTestNode(POINT pt, bool& onTextbox)
{
  onTextbox = false;

  for (uint32_t i = 0; i < m_nodeCount; i++) {
    VBehaviourNode *node = m_nodes[i];

    RECT r;
    GetWindowRect(node->hwnd(), &r);

    POINT tl = { r.left, r.top };
    ScreenToClient(hwnd(), &tl);

    RECT cr = { tl.x, tl.y, tl.x + NODE_W, tl.y + NODE_H };

    if (PtInRect(&cr, pt)) {
      // Check textboxes
      POINT screenPt = pt;
      ClientToScreen(hwnd(), &screenPt);

      RECT r0, r1;
      GetWindowRect(node->m_param0.hwnd(), &r0);
      GetWindowRect(node->m_param1.hwnd(), &r1);

      if (PtInRect(&r0, screenPt) || PtInRect(&r1, screenPt)) {
        onTextbox = true;
      }

      return node;
    }
  }

  return nullptr;
}

// --- main pressButton refactored ---
void VortexBehaviourEditor::pressButton(WPARAM wParam, LPARAM lParam)
{
  POINT pt = { LOWORD(lParam), HIWORD(lParam) };
  SetCapture(hwnd());

  // --- linking start ---
  VBehaviourNode *out = findOutputSocket(pt.x, pt.y);
  if (out) {
    m_linking = true;
    m_linkNode = out;
    m_linkMouseX = pt.x;
    m_linkMouseY = pt.y;

    selectNode(out);
    redraw();
    return;
  }

  // --- node hit test ---
  bool onTextbox = false;
  VBehaviourNode *hitNode = hitTestNode(pt, onTextbox);

  if (hitNode) {
    selectNode(hitNode);

    if (!onTextbox) {
      // Node background clicked: start drag
      RECT r;
      GetWindowRect(hitNode->hwnd(), &r);
      POINT tl = { r.left, r.top };
      ScreenToClient(hwnd(), &tl);

      m_dragNode = hitNode;
      m_dragOffsetX = pt.x - tl.x;
      m_dragOffsetY = pt.y - tl.y;
    }
  } else {
    // Clicked empty space: deselect everything
    deselectAllNodes();
    m_dragNode = nullptr;
  }

  redraw();
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

void VortexBehaviourEditor::deleteNode(VBehaviourNode* node)
{
  if (!node) return;

  // --- remove the behaviour from the engine ---
  BehaviourNode *bn = node->node();
  if (bn) {
    // remove it from the engine's behaviours
    // you might need a remove function in Behaviours
    m_engine.behaviours().removeNode(bn);
  }

  // --- remove the VBehaviourNode from editor array ---
  bool found = false;
  for (uint32_t i = 0; i < m_nodeCount; i++) {
    if (m_nodes[i] == node) {
      found = true;
      delete m_nodes[i];   // deletes window + textboxes
      // shift remaining nodes down
      for (uint32_t j = i; j < m_nodeCount - 1; j++)
        m_nodes[j] = m_nodes[j + 1];
      m_nodes[m_nodeCount - 1] = nullptr;
      m_nodeCount--;
      break;
    }
  }

  if (found)
    redraw();
}

void VortexBehaviourEditor::sendBehaviours()
{
  ByteStream data;
  m_engine.behaviours().serialize(data);

  VortexPort *port = nullptr;
  g_pEditor->getCurPort(&port);
  if (!port) {
    return;
  }

  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_SET_BEHAVIOUR);
  // wait for the ready
  port->expectData(EDITOR_VERB_READY);
  // recalc crc before sending
  data.recalcCRC();
  // send the behaviours
  port->writeData(data);
}
