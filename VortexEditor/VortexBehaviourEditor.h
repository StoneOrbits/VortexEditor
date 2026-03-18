#pragma once

#include "GUI/VChildWindow.h"
#include "GUI/VBehaviourNode.h"
#include "GUI/VBehaviourLinkOverlay.h"

#include "Behaviours/Behaviours.h"
#include "VortexEngine.h"

#include <windows.h>
#include <stdint.h>

class VortexBehaviourEditor : public VChildWindow
{
public:
  VortexBehaviourEditor(VortexEngine &engine);
  ~VortexBehaviourEditor();

  bool init(HINSTANCE inst);

  void show();
  void hide();
  void run();
  void redraw();

  void populateFromBehaviours();
  VBehaviourNode *findNodeFromBehaviour(BehaviourNode *bn);

  void drawLinks(HDC dc);
  void showContextMenu(int screenX, int screenY, int clientX, int clientY);

  virtual void paint() override;
  virtual void mouseMove(WPARAM wParam, LPARAM lParam) override;
  virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
  virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;
  virtual void rightClick(WPARAM wParam, LPARAM lParam) override;

  VBehaviourNode *findOutputSocket(int x, int y);
  VBehaviourNode *findInputSocket(int x, int y, int &socketIndex);

private:
  VortexEngine &m_engine;

  VBehaviourLinkOverlay m_behaviourLinkOverlay;

  HINSTANCE m_inst;

  bool m_isOpen;
  HICON m_hIcon;

  VBehaviourNode *m_nodes[MAX_BEHAVIOUR_NODES];
  uint32_t m_nodeCount;

  bool m_linking;
  VBehaviourNode *m_linkNode;
  int m_linkMouseX;
  int m_linkMouseY;

  VBehaviourNode *m_dragNode;
  int m_dragOffsetX;
  int m_dragOffsetY;
};