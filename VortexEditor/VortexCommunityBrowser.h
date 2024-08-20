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

  // show/hide the mode randomizer window
  void show();
  void hide();
  void loseFocus();
  json fetchModesJson(uint32_t page = 1, uint32_t pageSize = 15);
  bool loadCurPage(bool active = true);
  bool loadPage(uint32_t page, bool active = true);
  bool prevPage();
  bool nextPage();

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
  static void prevPageCallback(void *pthis, VWindow *window) {
    ((VortexCommunityBrowser *)pthis)->prevPage();
  }
  static void nextPageCallback(void *pthis, VWindow *window) {
    ((VortexCommunityBrowser *)pthis)->nextPage();
  }

  static DWORD __stdcall backgroundLoader(void *pthis);
  // a handle to the thread that initializes in the background
  HANDLE m_hThread;

  HINSTANCE m_hInstance;

  bool m_isOpen;

  HICON m_hIcon;

  // modes fetched from community api
  std::vector<nlohmann::json> m_communityModes;

  // mutex to synchronize access to vortex engine
  HANDLE m_mutex;
  // thread that runs the community browser vortex engine instance
  //HANDLE m_runThreadId;

  // child window for mode randomizer tool
  VChildWindow m_communityBrowserWindow;
  // preview of color
  std::vector<std::unique_ptr<VPatternStrip>> m_patternStrips;

  // next/prev page buttons
  VButton m_prevPageButton;
  VButton m_nextPageButton;
  VLabel m_pageLabel;

  // the current page of the browser
  uint32_t m_curPage;
  // whether we're on the last page (received all responses)
  bool m_lastPage;
};