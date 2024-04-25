#include "VortexCommunityBrowser.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

#include "HttpClient.h"

#include <winhttp.h>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

#define PREVIEW_ID 55501
#define PATTERN_STRIP_ID 55601

#define NEXT_PAGE_ID 55705
#define PREV_PAGE_ID 55706
#define PAGE_LABEL_ID 55707

// number of modes requested and displayed on each page
#define MODES_PER_PAGE 15

using namespace std;

VortexCommunityBrowser::VortexCommunityBrowser() :
  m_hThread(nullptr),
  m_hInstance(nullptr),
  m_isOpen(false),
  m_hIcon(nullptr),
  m_mutex(nullptr),
  m_communityBrowserWindow(),
  m_patternStrips(),
  m_prevPageButton(),
  m_nextPageButton(),
  m_pageLabel(),
  m_curPage(1),
  m_lastPage(false)
{
}

VortexCommunityBrowser::~VortexCommunityBrowser()
{
  DestroyIcon(m_hIcon);
}

// initialize the color picker
bool VortexCommunityBrowser::init(HINSTANCE hInst)
{
  m_hInstance = hInst;

  // the color picker
  m_communityBrowserWindow.init(hInst, "Vortex Community Browser", BACK_COL, 420, 690, this);
  m_communityBrowserWindow.setVisible(false);
  m_communityBrowserWindow.setCloseCallback(hideGUICallback);
  m_communityBrowserWindow.installLoseFocusCallback(loseFocusCallback);

  m_prevPageButton.init(hInst, m_communityBrowserWindow, "Prev", BACK_COL,
    64, 32, 100, 600, NEXT_PAGE_ID, prevPageCallback);
  m_nextPageButton.init(hInst, m_communityBrowserWindow, "Next", BACK_COL,
    64, 32, 256, 600, PREV_PAGE_ID, nextPageCallback);
  m_pageLabel.init(hInst, m_communityBrowserWindow, "1 / 1", BACK_COL,
    64, 32, 195, 600, PAGE_LABEL_ID, nullptr);

  // create stuff
  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE,
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_communityBrowserWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  // 15 modes per page
  for (uint32_t i = 0; i < MODES_PER_PAGE; ++i) {
    unique_ptr<VPatternStrip> strip = make_unique<VPatternStrip>(m_hInstance,
      m_communityBrowserWindow, "", BACK_COL,
      370, 32, 16, 16 + (i * 38), 2, json(), PATTERN_STRIP_ID + i, nullptr);
    m_patternStrips.push_back(move(strip));
  }

  m_hThread = CreateThread(NULL, 0, backgroundLoader, this, 0, NULL);

  return true;
}

DWORD __stdcall VortexCommunityBrowser::backgroundLoader(void *pthis)
{
  VortexCommunityBrowser *browser = (VortexCommunityBrowser *)pthis;
  Sleep(300);
  browser->loadPage();
  CloseHandle(browser->m_hThread);
  browser->m_hThread = nullptr;
  return 0;
}

void VortexCommunityBrowser::show()
{
  if (m_isOpen) {
    return;
  }
  m_communityBrowserWindow.setVisible(true);
  m_communityBrowserWindow.setEnabled(true);
  m_isOpen = true;
}

void VortexCommunityBrowser::hide()
{
  if (!m_isOpen) {
    return;
  }
  if (m_communityBrowserWindow.isVisible()) {
    m_communityBrowserWindow.setVisible(false);
  }
  if (m_communityBrowserWindow.isEnabled()) {
    m_communityBrowserWindow.setEnabled(false);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    g_pEditor->m_colorSelects[i].setSelected(false);
    g_pEditor->m_colorSelects[i].redraw();
  }
  m_isOpen = false;
}

void VortexCommunityBrowser::loseFocus()
{
}

json VortexCommunityBrowser::fetchModesJson(uint32_t page, uint32_t pageSize)
{
  HttpClient httpClient("VortexEditor/1.0");
  json result;
  try {
    map<string, string> queryParams = {
      { "page", to_string(page) },
      { "pageSize", to_string(pageSize) },
    };
    string jsonResponse = httpClient.SendRequest("vortex.community", "/modes/json", "GET", {}, "", queryParams);
    // Parse and return the JSON object
    result = json::parse(jsonResponse);
  } catch (const exception &e) {
    cerr << "Exception caught: " << e.what() << endl;
  }
  return result;
}

bool VortexCommunityBrowser::loadPage()
{
  // fetch json of modes from community api
  try {
    m_communityModes = fetchModesJson(m_curPage, MODES_PER_PAGE);
  } catch (...) {
    return false;
  }
  // Verify if 'data' is an array
  if (!m_communityModes.contains("data")) {
    cerr << "'data' is not an array or does not exist" << endl;
    return false;
  }
  for (uint32_t i = 0; i < MODES_PER_PAGE; ++i) {
    auto mode = m_communityModes["data"][i];
    m_patternStrips[i]->loadJson(mode);
  }
  uint32_t numPages = m_communityModes["pages"];
  m_lastPage = (m_curPage == numPages);
  m_pageLabel.setText(to_string(m_curPage) + " / " + to_string(numPages));
  return true;
}

bool VortexCommunityBrowser::prevPage()
{
  if (m_curPage <= 1) {
    return false;
  }
  m_curPage--;
  return loadPage();
}

bool VortexCommunityBrowser::nextPage()
{
  if (m_lastPage) {
    return false;
  }
  m_curPage++;
  return loadPage();
}
