#pragma once

// windows includes
#include <windows.h>

// gui includes
#include "GUI/VChildwindow.h"
#include "GUI/VColorSelect.h"
#include "GUI/VSelectBox.h"
#include "GUI/VComboBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

#include "Colors/Colortypes.h"

#include "VortexPort.h"

class VortexChromaLink
{
public:
  // Duo save header
  struct HeaderData
  {
    HeaderData() :
      vMajor(0), vMinor(0), globalFlags(0),
      brightness(0), numModes(0)
    {
    }
    uint8_t vMajor;
    uint8_t vMinor;
    uint8_t globalFlags;
    uint8_t brightness;
    uint8_t numModes;
  };

  VortexChromaLink();
  ~VortexChromaLink();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the mode randomizer
  void run();

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();
  void connectLink();
  void pullDuoModes();
  void pushDuoModes();
  void flashFirmware();
  void pullSingleDuoMode();
  void pushSingleDuoMode();

  bool pullHeader(HeaderData &headerData);

  bool isOpen() const { return m_isOpen; }
  
  HWND hwnd() const { return m_chromaLinkWindow.hwnd(); }

private:
  // ==================================
  //  Mode Randomizer GUI
  static void hideGUICallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->hide();
  }
  static void loseFocusCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->loseFocus();
  }
  static void connectCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->connectLink();
  }
  static void pullCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pullDuoModes();
  }
  static void pushCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pushDuoModes();
  }
  static void pullSingleCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pullSingleDuoMode();
  }
  static void pushSingleCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->pushSingleDuoMode();
  }

  static void flashCallback(void *pthis, VWindow *window) {
    ((VortexChromaLink *)pthis)->flashFirmware();
  }

  bool m_isOpen;

  HICON m_hIcon;

  // child window for mode randomizer tool
  VChildWindow m_chromaLinkWindow;

  // pull from the connected duo via chromadeck
  VButton m_connectButton;
  VButton m_pullButton;
  VButton m_pushButton;
  VButton m_flashButton;
  VButton m_pullSingleButton;
  VButton m_pushSingleButton;

  // connected duo header
  HeaderData m_connectedDuo;
  // and the port it's connected through
  VortexPort *m_connectedPort;
};
