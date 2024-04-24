#include "VortexEditorTutorial.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

using namespace std;

VortexEditorTutorial::VortexEditorTutorial() :
  m_isOpen(false),
  m_hIcon(nullptr),
  m_script(),
  m_currentScriptIndex(0),
  m_nextPressed(false),
  m_mutex(nullptr),
  m_runThread(nullptr),
  m_tutorialWindow(),
  m_tutorialStatus(),
  m_orbitBitmap(),
  m_orbitSelectedBitmap(),
  m_orbitSelect(),
  m_handlesBitmap(),
  m_handlesSelectedBitmap(),
  m_handlesSelect(),
  m_glovesBitmap(),
  m_glovesSelectedBitmap(),
  m_glovesSelect(),
  m_chromadeckBitmap(),
  m_chromadeckSelectedBitmap(),
  m_chromadeckSelect()
{
}

VortexEditorTutorial::~VortexEditorTutorial()
{
  DestroyIcon(m_hIcon);
}

// initialize the color picker
bool VortexEditorTutorial::init(HINSTANCE hInst)
{
  // the color picker
  m_tutorialWindow.init(hInst, "Vortex Editor Tutorial", BACK_COL, 420, 240, this);
  m_tutorialWindow.setVisible(false);
  m_tutorialWindow.setCloseCallback(hideGUICallback);
  m_tutorialWindow.installLoseFocusCallback(loseFocusCallback);

  // status bar
  m_tutorialStatus.init(hInst, m_tutorialWindow, "", BACK_COL, 375, 24, 10, 16, 0, nullptr);
  m_tutorialStatus.setForeEnabled(true);
  m_tutorialStatus.setBackEnabled(true);
  m_tutorialStatus.setStatus(RGB(30, 164, 153), "Welcome to the Vortex Editor Tutorial");

  // load bitmaps for buttons
  m_chromadeckBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_glovesBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_handlesBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP3), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_orbitBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

  m_chromadeckSelectedBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP5), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_glovesSelectedBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP6), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_handlesSelectedBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP7), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
  m_orbitSelectedBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP8), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

  // the orbit select
  m_orbitSelect.init(hInst, m_tutorialWindow, "orbit", RGB(20, 20, 20), 64, 64, 34, 100, 123123, selectOrbitCallback);
  m_orbitSelect.setVisible(true);
  m_orbitSelect.setEnabled(true);
  m_orbitSelect.setBackground(m_orbitBitmap);
  m_orbitSelect.setHoverBackground(m_orbitSelectedBitmap);
  m_orbitSelect.setBackgroundTransparency(true);
  m_orbitSelect.setDrawCircle(false);
  m_orbitSelect.setDrawHLine(false);
  m_orbitSelect.setDrawVLine(false);
  m_orbitSelect.setBorderSize(0);

  // the handles select
  m_handlesSelect.init(hInst, m_tutorialWindow, "handles", BACK_COL, 64, 64, 124, 100, 123124, selectOrbitCallback);
  m_handlesSelect.setVisible(true);
  m_handlesSelect.setEnabled(true);
  m_handlesSelect.setBackground(m_handlesBitmap);
  m_handlesSelect.setHoverBackground(m_handlesSelectedBitmap);
  m_handlesSelect.setBackgroundTransparency(false);
  m_handlesSelect.setDrawCircle(false);
  m_handlesSelect.setDrawHLine(false);
  m_handlesSelect.setDrawVLine(false);
  m_handlesSelect.setBorderSize(0);

  // the gloves select
  m_glovesSelect.init(hInst, m_tutorialWindow, "gloves", BACK_COL, 64, 64, 214, 100, 123125, selectOrbitCallback);
  m_glovesSelect.setVisible(true);
  m_glovesSelect.setEnabled(true);
  m_glovesSelect.setBackground(m_glovesBitmap);
  m_glovesSelect.setHoverBackground(m_glovesSelectedBitmap);
  m_glovesSelect.setBackgroundTransparency(false);
  m_glovesSelect.setDrawCircle(false);
  m_glovesSelect.setDrawHLine(false);
  m_glovesSelect.setDrawVLine(false);
  m_glovesSelect.setBorderSize(0);

  // the chromadeck select
  m_chromadeckSelect.init(hInst, m_tutorialWindow, "chromadeck", BACK_COL, 64, 64, 304, 100, 123126, selectOrbitCallback);
  m_chromadeckSelect.setVisible(true);
  m_chromadeckSelect.setEnabled(true);
  m_chromadeckSelect.setBackground(m_chromadeckBitmap);
  m_chromadeckSelect.setHoverBackground(m_chromadeckSelectedBitmap);
  m_chromadeckSelect.setBackgroundTransparency(false);
  m_chromadeckSelect.setDrawCircle(false);
  m_chromadeckSelect.setDrawHLine(false);
  m_chromadeckSelect.setDrawVLine(false);
  m_chromadeckSelect.setBorderSize(0);

  // create stuff
  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_tutorialWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  m_script = {
     "Welcome to the Vortex Editor Tutorial",
     "Which Vortex Device will you be using?",
  };

  return true;
}

void VortexEditorTutorial::run()
{
}

void VortexEditorTutorial::show()
{
  if (m_isOpen) {
    return;
  }
  m_tutorialWindow.setVisible(true);
  m_tutorialWindow.setEnabled(true);
  m_isOpen = true;

  if (!m_runThread) {
    // start the runthread
    m_runThread = CreateThread(NULL, 0, runThread, this, 0, NULL);
  }
}

void VortexEditorTutorial::hide()
{
  if (!m_isOpen) {
    return;
  }
  if (m_tutorialWindow.isVisible()) {
    m_tutorialWindow.setVisible(false);
  }
  if (m_tutorialWindow.isEnabled()) {
    m_tutorialWindow.setEnabled(false);
  }
  m_isOpen = false;
}

void VortexEditorTutorial::loseFocus()
{
}

DWORD __stdcall VortexEditorTutorial::runThread(void *arg)
{
  VortexEditorTutorial *editorTutorial = (VortexEditorTutorial *)arg;
  while (editorTutorial->m_currentScriptIndex < editorTutorial->m_script.size()) {
    WaitForSingleObject(editorTutorial->m_mutex, INFINITE);
    if (editorTutorial->m_nextPressed) {
      editorTutorial->m_nextPressed = false;
      editorTutorial->m_currentScriptIndex++;
      if (editorTutorial->m_currentScriptIndex >= editorTutorial->m_script.size()) {
        ReleaseMutex(editorTutorial->m_mutex);
        break;
      }
    }
    ReleaseMutex(editorTutorial->m_mutex);

    const std::string &message = editorTutorial->m_script[editorTutorial->m_currentScriptIndex];
    std::string msg;
    for (size_t i = 0; i < message.length(); ++i) {
      Sleep(40); // Simulate typing delay
      msg += message[i];
      // Place your method to update the text in UI here
      editorTutorial->UpdateTextDisplay(msg.c_str());
      WaitForSingleObject(editorTutorial->m_mutex, INFINITE);
      if (editorTutorial->m_nextPressed) {
        ReleaseMutex(editorTutorial->m_mutex);
        break; // Move to the next message if "next" is pressed
      }
      ReleaseMutex(editorTutorial->m_mutex);
    }

    // Wait a bit before proceeding to the next message automatically
    Sleep(1000); // Adjust as needed
    editorTutorial->NextMessageAutomatically();
  }
  return 0;
}

void VortexEditorTutorial::StartTutorial()
{
  m_currentScriptIndex = 0;
  // Start the thread that displays the first message
  m_runThread = CreateThread(nullptr, 0, runThread, this, 0, nullptr);
}

void VortexEditorTutorial::NextMessage()
{
  // Advances to the next message when the 'next' button is clicked
  WaitForSingleObject(m_mutex, INFINITE);
  if (m_currentScriptIndex + 1 < m_script.size()) {
    m_nextPressed = true; // Signal to proceed to the next message
  }
  ReleaseMutex(m_mutex);
}

void VortexEditorTutorial::UpdateTextDisplay(const char *text)
{
  m_tutorialStatus.setText(text);
}

void VortexEditorTutorial::NextMessageAutomatically()
{
  // Automatically proceed to the next message or handle end of script
  WaitForSingleObject(m_mutex, INFINITE);
  if (!m_nextPressed && m_currentScriptIndex + 1 < m_script.size()) {
    m_nextPressed = true;
  }
  ReleaseMutex(m_mutex);
}
