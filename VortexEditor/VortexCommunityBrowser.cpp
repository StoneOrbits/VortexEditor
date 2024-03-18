#include "VortexCommunityBrowser.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

#include <winhttp.h>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

#define PREVIEW_ID 55501
#define PATTERN_STRIP_ID 55601

using namespace std;
using json = nlohmann::json;

VortexCommunityBrowser::VortexCommunityBrowser() :
  m_isOpen(false),
  m_hIcon(nullptr),
  m_mutex(nullptr),
  m_runThreadId(nullptr),
  m_communityBrowserWindow(),
  m_patternStrips()
{
}

VortexCommunityBrowser::~VortexCommunityBrowser()
{
  DestroyIcon(m_hIcon);
  TerminateThread(m_runThreadId, 0);
}

// initialize the color picker
bool VortexCommunityBrowser::init(HINSTANCE hInst)
{
  // the color picker
  m_communityBrowserWindow.init(hInst, "Vortex Community Browser", BACK_COL, 420, 690, this);
  m_communityBrowserWindow.setVisible(false);
  m_communityBrowserWindow.setCloseCallback(hideGUICallback);
  m_communityBrowserWindow.installLoseFocusCallback(loseFocusCallback);

  // fetch json of modes from community api
  try {
    m_communityModes = json::parse(GetHttpRequest(L"vortex.community", L"/modes/json"));
  } catch (...) {
    return false;
  }

  // Verify if 'data' is an array
  if (!m_communityModes.contains("data")) {
    std::cerr << "'data' is not an array or does not exist" << std::endl;
    return false;
  }

  uint32_t i = 0;
  for (auto mode : m_communityModes["data"]) {
    if (!mode.contains("modeData")) {
      //continue;
    }
    shared_ptr<VPatternStrip> strip = make_shared<VPatternStrip>(hInst, 
      m_communityBrowserWindow, "Mode " + to_string(i), BACK_COL,
      300, 32, 16, 16 + (i * 36), 2, mode["modeData"], PATTERN_STRIP_ID + i, nullptr);
    m_patternStrips.push_back(move(strip));
    i++;
  }

  // create stuff
  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE,
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_communityBrowserWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  m_runThreadId  = CreateThread(NULL, 0, runThread, this, 0, NULL);

  return true;
}

DWORD __stdcall VortexCommunityBrowser::runThread(void *arg)
{
  VortexCommunityBrowser *browser = (VortexCommunityBrowser *)arg;
  while (1) {
    if (browser->m_isOpen) {
      browser->run();
    }
  }
  return 0;
}

void VortexCommunityBrowser::run()
{
  for (auto strip : m_patternStrips) {
    strip.get()->run();
  }
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

std::wstring VortexCommunityBrowser::GetHttpRequest(const std::wstring& host, const std::wstring& path)
{
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  LPSTR pszOutBuffer;
  std::wstring result;
  BOOL  bResults = FALSE;
  HINTERNET  hSession = NULL,
    hConnect = NULL,
    hRequest = NULL;

  // Use WinHttpOpen to obtain a session handle.
  hSession = WinHttpOpen(L"A Vortex Editor/1.0",
    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    WINHTTP_NO_PROXY_NAME,
    WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) {
    return L"";
  }
  hConnect = WinHttpConnect(hSession, host.c_str(),
    INTERNET_DEFAULT_HTTP_PORT, 0);
  // Create an HTTP request handle.
  if (!hConnect) {
    return L"";
  }
  hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
    NULL, WINHTTP_NO_REFERER,
    WINHTTP_DEFAULT_ACCEPT_TYPES,
    0);
  // Send a request.
  if (!hRequest) {
    return L"";
  }
  bResults = WinHttpSendRequest(hRequest,
    WINHTTP_NO_ADDITIONAL_HEADERS, 0,
    WINHTTP_NO_REQUEST_DATA, 0,
    0, 0);

  // End the request.
  if (!bResults) {
    return L"";
  }
  bResults = WinHttpReceiveResponse(hRequest, NULL);

  // Keep checking for data until there is nothing left.
  if (!bResults) {
    return L"";
  }
  do {
    // Check for available data.
    dwSize = 0;
    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
      //std::wcout << L"Error " << GetLastError() << L" in WinHttpQueryDataAvailable.\n";
    }

    // Allocate space for the buffer.
    pszOutBuffer = new char[dwSize + 1];
    if (!pszOutBuffer) {
      //std::wcout << L"Out of memory\n";
      dwSize = 0;
      return L"";
    }
    // Read the data.
    ZeroMemory(pszOutBuffer, dwSize + 1);
    if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
      delete[] pszOutBuffer;
      return L"";
    }
    result.append(pszOutBuffer, pszOutBuffer + dwDownloaded);
    // Free the memory allocated to the buffer.
    delete[] pszOutBuffer;
  } while (dwSize > 0);

  // Close any open handles.
  if (hRequest) {
    WinHttpCloseHandle(hRequest);
  }
  if (hConnect) {
    WinHttpCloseHandle(hConnect);
  }
  if (hSession) {
    WinHttpCloseHandle(hSession);
  }

  if (result.empty()) {
    return std::wstring();
  }
  return result;
}
