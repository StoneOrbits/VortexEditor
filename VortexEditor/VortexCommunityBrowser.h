#pragma once

// windows includes
#include <windows.h>

// gui includes
#include "GUI/VChildwindow.h"
#include "GUI/VPatternStrip.h"
#include "GUI/VSelectBox.h"
#include "GUI/VComboBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

#include "Colors/Colortypes.h"

// engine includes
#include "VortexLib.h"

#include "json.hpp"

class VortexCommunityBrowser
{
public:
  VortexCommunityBrowser();
  ~VortexCommunityBrowser();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the mode randomizer
  void run();

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();
  std::wstring GetHttpRequest(const std::wstring &host, const std::wstring &path);

  bool isOpen() const { return m_isOpen; }
  
  HWND hwnd() const { return m_communityBrowserWindow.hwnd(); }

private:
  // ==================================
  //  Mode Randomizer GUI
  static void hideGUICallback(void *pthis, VWindow *window) {
    ((VortexCommunityBrowser *)pthis)->hide();
  }
  static void loseFocusCallback(void *pthis, VWindow *window) {
    ((VortexCommunityBrowser *)pthis)->loseFocus();
  }

  static DWORD __stdcall runThread(void *arg);

  bool m_isOpen;

  HICON m_hIcon;

  // modes fetched from community api
  nlohmann::json m_communityModes;

  // mutex to synchronize access to vortex engine
  HANDLE m_mutex;
  // thread that runs the community browser vortex engine instance
  HANDLE m_runThreadId;

  // child window for mode randomizer tool
  VChildWindow m_communityBrowserWindow;
  // preview of color
  std::vector<std::shared_ptr<VPatternStrip>> m_patternStrips;
};