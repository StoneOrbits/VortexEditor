#include "VortexBehaviourEditor.h"

#include "VortexEditor.h"
#include "EditorConfig.h"

#include "resource.h"

VortexBehaviourEditor::VortexBehaviourEditor(VortexEngine &engine) :
  m_engine(engine),
  m_isOpen(false),
  m_hIcon(nullptr),
  m_window(),
  m_behaviourEditor(engine)
{
}

VortexBehaviourEditor::~VortexBehaviourEditor()
{
  if (m_hIcon) {
    DestroyIcon(m_hIcon);
  }
}

bool VortexBehaviourEditor::init(HINSTANCE hInst)
{
  m_window.init(hInst, "Behaviour Editor", BACK_COL, 900, 600, this);
  m_window.setVisible(false);
  m_window.setEnabled(false);

  m_window.setCloseCallback(hideGUICallback);
  m_window.installLoseFocusCallback(loseFocusCallback);

  RECT rc;
  GetClientRect(m_window.hwnd(), &rc);

  m_behaviourEditor.init(
    hInst,
    m_window.hwnd(),
    0,
    0,
    rc.right - rc.left,
    rc.bottom - rc.top
  );

  m_behaviourEditor.populateFromBehaviours();

  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_window.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
}

void VortexBehaviourEditor::run()
{
}

void VortexBehaviourEditor::show()
{
  if (m_isOpen) {
    return;
  }

  m_window.setVisible(true);
  m_window.setEnabled(true);

  m_behaviourEditor.populateFromBehaviours();

  m_isOpen = true;
}

void VortexBehaviourEditor::hide()
{
  if (!m_isOpen) {
    return;
  }

  if (m_window.isVisible()) {
    m_window.setVisible(false);
  }

  if (m_window.isEnabled()) {
    m_window.setEnabled(false);
  }

  m_isOpen = false;
}

void VortexBehaviourEditor::loseFocus()
{
}

void VortexBehaviourEditor::hideGUICallback(void *arg, VWindow *window)
{
  VortexBehaviourEditor *self = (VortexBehaviourEditor *)arg;
  if (self) {
    self->hide();
  }
}

void VortexBehaviourEditor::loseFocusCallback(void *arg, VWindow *window)
{
  VortexBehaviourEditor *self = (VortexBehaviourEditor *)arg;
  if (self) {
    self->loseFocus();
  }
}