#ifndef VORTEX_BEHAVIOUR_EDITOR_H
#define VORTEX_BEHAVIOUR_EDITOR_H

#include <Windows.h>

#include "GUI/VChildwindow.h"
#include "BehaviourEditor.h"

class VortexEngine;

class VortexBehaviourEditor
{
public:

  VortexBehaviourEditor(VortexEngine &engine);
  ~VortexBehaviourEditor();

  bool init(HINSTANCE hInst);

  void show();
  void hide();
  void run();

  HWND hwnd() const { return m_behaviourEditor.hwnd(); }

private:

  static void hideGUICallback(void *arg, VWindow *window);
  static void loseFocusCallback(void *arg, VWindow *window);

  void loseFocus();

private:

  VortexEngine &m_engine;
  bool m_isOpen;

  HICON m_hIcon;

  VChildWindow m_window;
  BehaviourEditor m_behaviourEditor;
};

#endif
