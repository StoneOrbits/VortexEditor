#include "VortexEditor.h"
#include "EditorConfig.h"

#include "GUI/VWindow.h"

#include "resource.h"

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
  m_hInstance(NULL)
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
  m_hInstance = hInstance;
  // initialize the window accordingly
  m_window.init(hInstance, EDITOR_TITLE, EDITOR_BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT);
  m_pushButton.init(hInstance, m_window, "Push", EDITOR_BACK_COL, 72, 28, 16, 16, ID_FILE_PUSH, pushCallback);
  m_pullButton.init(hInstance, m_window, "Pull", EDITOR_BACK_COL, 72, 28, 16, 48, ID_FILE_PULL, pullCallback);
  m_loadButton.init(hInstance, m_window, "Load", EDITOR_BACK_COL, 72, 28, 16, 80, ID_FILE_LOAD, loadCallback);
  m_saveButton.init(hInstance, m_window, "Save", EDITOR_BACK_COL, 72, 28, 16, 112, ID_FILE_SAVE, saveCallback);
  return true;
}

void VortexEditor::run()
{
  // main message loop
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    // pass message to main window otherwise process it
    if (!m_window.process(msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

void VortexEditor::pushCallback(const VButton &button)
{
  MessageBox(0, "Push", "", 0);
}

void VortexEditor::pullCallback(const VButton &button)
{
  MessageBox(0, "Pull", "", 0);
}

void VortexEditor::loadCallback(const VButton &button)
{
  MessageBox(0, "Load", "", 0);
}

void VortexEditor::saveCallback(const VButton &button)
{
  MessageBox(0, "Save", "", 0);
}
