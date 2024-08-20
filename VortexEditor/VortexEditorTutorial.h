#pragma once

// windows includes
#include <windows.h>

// gui includes
#include "GUI/VChildwindow.h"
#include "GUI/VColorSelect.h"
#include "GUI/VSelectBox.h"
#include "GUI/VStatusBar.h"
#include "GUI/VComboBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

#include "Colors/Colortypes.h"

#include <vector>

class VortexEditorTutorial
{
public:
  VortexEditorTutorial();
  ~VortexEditorTutorial();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the mode randomizer
  void run();

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();
  void StartTutorial();
  void NextMessage();

  void UpdateTextDisplay(const char *text);
  void NextMessageAutomatically();

  bool isOpen() const { return m_isOpen; }
  
  HWND hwnd() const { return m_tutorialWindow.hwnd(); }

private:
  // ==================================
  //  Mode Randomizer GUI
  static void hideGUICallback(void *pthis, VWindow *window) {
    ((VortexEditorTutorial *)pthis)->hide();
  }
  static void loseFocusCallback(void *pthis, VWindow *window) {
    ((VortexEditorTutorial *)pthis)->loseFocus();
  }
  // callbacks for selecting sv and h
  static void selectOrbitCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent) {
    ((VortexEditorTutorial *)pthis)->selectOrbit(x, y, sevent); 
  }
  // callbacks for selecting sv and h
  static void selectHandleCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent) {
    ((VortexEditorTutorial *)pthis)->selectHandle(x, y, sevent); 
  }
  // callbacks for selecting sv and h
  static void selectGlovesCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent) {
    ((VortexEditorTutorial *)pthis)->selectGloves(x, y, sevent); 
  }
  // callbacks for selecting sv and h
  static void selectChromadeckCallback(void *pthis, uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent) {
    ((VortexEditorTutorial *)pthis)->selectChromadeck(x, y, sevent); 
  }

  void selectOrbit(uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent);
  void selectHandle(uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent);
  void selectGloves(uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent);
  void selectChromadeck(uint32_t x, uint32_t y, VSelectBox::SelectEvent sevent);

  enum SelectedDevice {
    SELECTED_NONE,

    SELECTED_ORBIT,
    SELECTED_HANDLE,
    SELECTED_GLOVES,
    SELECTED_CHROMADECK
  };

  SelectedDevice m_selectedDevice;

  static DWORD __stdcall runThread(void *arg);

  bool m_isOpen;

  HICON m_hIcon;

  std::vector<std::string> m_script;
  uint32_t m_currentScriptIndex;
  bool m_nextPressed;

  // mutex that is posted once we're loaded
  HANDLE m_mutex;
  HANDLE m_runThread;

  // child window for mode randomizer tool
  VChildWindow m_tutorialWindow;

  // textbox to print out messages
  VStatusBar m_tutorialStatus;

  // the orbit button/selection
  HBITMAP m_orbitBitmap;
  HBITMAP m_orbitSelectedBitmap;
  VSelectBox m_orbitSelect;

  // the handles button/selection
  HBITMAP m_handlesBitmap;
  HBITMAP m_handlesSelectedBitmap;
  VSelectBox m_handlesSelect;

  // the gloves button/selection
  HBITMAP m_glovesBitmap;
  HBITMAP m_glovesSelectedBitmap;
  VSelectBox m_glovesSelect;

  // the chromadeck button/selection
  HBITMAP m_chromadeckBitmap;
  HBITMAP m_chromadeckSelectedBitmap;
  VSelectBox m_chromadeckSelect;
};
