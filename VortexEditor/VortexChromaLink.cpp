#include "VortexChromaLink.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"
#include "Modes/Mode.h"

#include "resource.h"

#include "Serial/Compression.h"
#include "VortexPort.h"

#define CHROMALINK_PULL_ID 58001

using namespace std;

VortexChromaLink::VortexChromaLink() :
  m_isOpen(false),
  m_hIcon(nullptr),
  m_chromaLinkWindow()
{
}

VortexChromaLink::~VortexChromaLink()
{
  DestroyIcon(m_hIcon);
}

// initialize the color picker
bool VortexChromaLink::init(HINSTANCE hInst)
{
  // the color picker
  m_chromaLinkWindow.init(hInst, "Vortex Chroma Link", BACK_COL, 369, 242, this);
  m_chromaLinkWindow.setVisible(false);
  m_chromaLinkWindow.setCloseCallback(hideGUICallback);
  m_chromaLinkWindow.installLoseFocusCallback(loseFocusCallback);

  // create stuff

  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  m_pullButton.init(hInst, m_chromaLinkWindow, "Pull Duo", BACK_COL,
    64, 28, 10, 10, CHROMALINK_PULL_ID, pullCallback);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_chromaLinkWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
}

void VortexChromaLink::run()
{
}

void VortexChromaLink::show()
{
  if (m_isOpen) {
    return;
  }
  m_chromaLinkWindow.setVisible(true);
  m_chromaLinkWindow.setEnabled(true);
  m_isOpen = true;
}

void VortexChromaLink::hide()
{
  if (!m_isOpen) {
    return;
  }
  if (m_chromaLinkWindow.isVisible()) {
    m_chromaLinkWindow.setVisible(false);
  }
  if (m_chromaLinkWindow.isEnabled()) {
    m_chromaLinkWindow.setEnabled(false);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    //g_pEditor->m_colorSelects[i].setSelected(false);
    //g_pEditor->m_colorSelects[i].redraw();
  }
  m_isOpen = false;
}

void VortexChromaLink::loseFocus()
{
}

void VortexChromaLink::pullDuoMode()
{
  VortexPort *port = nullptr;
  if (!g_pEditor->isConnected() || !g_pEditor->getCurPort(&port)) {
    return;
  }
  ByteStream headerBuffer;
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_PULL_CHROMA_HDR);
  headerBuffer.clear();
  if (!port->readByteStream(headerBuffer) || !headerBuffer.size()) {
    debug("Couldn't read anything");
    return;
  }
  // now send the done message
  port->writeData(EDITOR_VERB_PULL_MODES_DONE);
  //g_pEditor->m_vortex.matchLedCount(stream, false);
  //g_pEditor->m_vortex.setModes(stream);
  // wait for the ack
  port->expectData(EDITOR_VERB_PULL_CHROMA_HDR_ACK);

  struct HeaderData
  {
    uint8_t vMajor;
    uint8_t vMinor;
    uint8_t globalFlags;
    uint8_t brightness;
    uint8_t numModes;
  };
  // quick way to interpret the data
  HeaderData *headerData = (HeaderData *)headerBuffer.data();
  if (!headerData) {
    return;
  }
  if (headerData->vMajor != 1 || headerData->vMinor != 2) {
    return;
  }
  g_pEditor->m_vortex.setLedCount(2);
  g_pEditor->m_vortex.engine().modes().clearModes();
  for (uint8_t i = 0; i < headerData->numModes; ++i) {
    ByteStream modeBuffer;
    // tell it to send a mode
    port->writeData(EDITOR_VERB_PULL_CHROMA_MODE);
    // it's ready for the mode idx
    port->expectData(EDITOR_VERB_READY);
    // send the mode idx
    port->writeData((uint8_t *)&i, 1);
    // read the mode
    modeBuffer.clear();
    if (!port->readByteStream(modeBuffer) || !modeBuffer.size()) {
      debug("Couldn't read anything");
      return;
    }
    // now send the done message
    port->writeData(EDITOR_VERB_PULL_MODES_DONE);
    // wait for the ack from the gloves
    port->expectData(EDITOR_VERB_PULL_CHROMA_MODE_ACK);
    Mode newMode(g_pEditor->m_engine);
    newMode.unserialize(modeBuffer);
    // add the mode
    g_pEditor->m_vortex.addMode(&newMode);
  }
  // refresh the mode list
  g_pEditor->refreshModeList();
  // demo the current mode
  g_pEditor->demoCurMode();
}

