#include "VortexChromaLink.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"
#include "Modes/Mode.h"

#include "resource.h"

#include "Serial/Compression.h"
#include "VortexPort.h"

#include <fstream>
#include <vector>
#include <thread>
#include <chrono>

#define CHROMALINK_CONNECT_ID 58000
#define CHROMALINK_PULL_ID 58001
#define CHROMALINK_PUSH_ID 58002
#define CHROMALINK_FLASH_ID 58003
#define CHROMALINK_PULL_SINGLE_ID 58004
#define CHROMALINK_PUSH_SINGLE_ID 58005

#define FIRMWARE_TRANSFER_BLOCK_SIZE 128

using namespace std;

VortexChromaLink::VortexChromaLink() :
  m_isOpen(false),
  m_hIcon(nullptr),
  m_chromaLinkWindow(),
  m_connectedDuo(),
  m_connectedPort(nullptr)
{
}

VortexChromaLink::~VortexChromaLink()
{
  DestroyIcon(m_hIcon);
}

// initialize the color picker
bool VortexChromaLink::init(HINSTANCE hInst)
{
  // the chromalink window
  m_chromaLinkWindow.init(hInst, "Vortex Chroma Link", BACK_COL, 369, 242, this);
  m_chromaLinkWindow.setVisible(true);
  m_chromaLinkWindow.setCloseCallback(hideGUICallback);
  m_chromaLinkWindow.installLoseFocusCallback(loseFocusCallback);

  // create stuff
  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  m_connectButton.init(hInst, m_chromaLinkWindow, "Connect Link", BACK_COL,
    120, 28, 10, 10, CHROMALINK_CONNECT_ID, connectCallback);
  m_pullButton.init(hInst, m_chromaLinkWindow, "Pull Modes", BACK_COL,
    120, 28, 10, 48, CHROMALINK_PULL_ID, pullCallback);
  m_pushButton.init(hInst, m_chromaLinkWindow, "Push Modes", BACK_COL,
    120, 28, 10, 86, CHROMALINK_PUSH_ID, pushCallback);
  m_flashButton.init(hInst, m_chromaLinkWindow, "Flash Firmware", BACK_COL,
    120, 28, 10, 124, CHROMALINK_FLASH_ID, flashCallback);

  m_pullSingleButton.init(hInst, m_chromaLinkWindow, "Pull Single Mode", BACK_COL,
    130, 28, 140, 48, CHROMALINK_PULL_SINGLE_ID, pullSingleCallback);
  m_pushSingleButton.init(hInst, m_chromaLinkWindow, "Push Single Mode", BACK_COL,
    130, 28, 140, 86, CHROMALINK_PUSH_SINGLE_ID, pushSingleCallback);

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

bool VortexChromaLink::pullHeader(HeaderData &headerData)
{
  if (!m_connectedPort) {
    g_pEditor->setStatus(255, 0, 0, "Error: Not connected.");
    return false;
  }
  if (!m_connectedPort->writeData(EDITOR_VERB_PULL_CHROMA_HDR)) {
    g_pEditor->setStatus(255, 0, 0, "Error: Could not write pull header request.");
    return false;
  }
  g_pEditor->setStatus(0, 255, 0, "Waiting for header...");
  ByteStream headerBuffer;
  if (!m_connectedPort->readByteStream(headerBuffer) || !headerBuffer.size()) {
    g_pEditor->setStatus(255, 0, 0, "Error: Could not read header.");
    return false;
  }
  g_pEditor->setStatus(0, 255, 0, "Checking header CRC...");
  if (!headerBuffer.checkCRC() || headerBuffer.size() < sizeof(headerData)) {
    g_pEditor->setStatus(255, 0, 0, "Error: Header CRC or size invalid.");
    return false;
  }
  memcpy(&headerData, (HeaderData *)headerBuffer.data(), sizeof(headerData));
  return true;
}

void VortexChromaLink::connectLink()
{
  thread([this]() {
    g_pEditor->setStatus(255, 255, 0, "Starting connection...");
    // first establish the port that is connected to the chromadeck
    if (!g_pEditor->isConnected() || !g_pEditor->getCurPort(&m_connectedPort) || !m_connectedPort) {
      g_pEditor->setStatus(255, 0, 0, "No connection.");
      return;
    }
    // then pull the header from the duo via that chromadeck
    g_pEditor->setStatus(0, 255, 0, "Connected. Pulling header...");
    // TODO: lock on connectedDuo since this is separate thread
    if (!pullHeader(m_connectedDuo)) {
      // failure, pullHeader sets a status message
      return;
    }
    // detect vmajor 0?
    if (!m_connectedDuo.vMajor) {
      string msg = "WARNING: Connected Duo v" + to_string(m_connectedDuo.vMajor) +
        "." + to_string(m_connectedDuo.vMinor) + " - Major version 0! MUST FLASH FIRMWARE!";
      return;
    }
    // success connected
    string msg = "Connected Duo v" + to_string(m_connectedDuo.vMajor) +
      "." + to_string(m_connectedDuo.vMinor);
    g_pEditor->setStatus(0, 255, 0, msg);
  }).detach();
}

void VortexChromaLink::pullDuoModes()
{
  thread([this]() {
    // make sure there is a valid duo connected
    if (!m_connectedDuo.vMajor || !m_connectedPort) {
      g_pEditor->setStatus(255, 0, 0, "Connect a Duo first");
      return;
    }

    g_pEditor->setStatus(255, 255, 0, "Pulling modes from Duo...");

    // check version?
    //if (headerData->vMajor != 1 || headerData->vMinor != 2) {
    //  g_pEditor->setStatus(255, 0, 0, "Error: Invalid header.");
    //  return;
    //}

    g_pEditor->setStatus(255, 255, 0, "Reading modes...");
    g_pEditor->m_vortex.setLedCount(2);
    g_pEditor->m_vortex.engine().modes().clearModes();

    for (uint8_t i = 0; i < m_connectedDuo.numModes; ++i) {
      ByteStream modeBuffer;
      m_connectedPort->writeData(EDITOR_VERB_PULL_CHROMA_MODE);
      g_pEditor->setStatus(255, 255, 0, "Reading mode " + to_string(i) + "...");

      m_connectedPort->expectData(EDITOR_VERB_READY);
      m_connectedPort->writeData((uint8_t *)&i, 1);

      modeBuffer.clear();
      if (!m_connectedPort->readByteStream(modeBuffer) || !modeBuffer.size()) {
        g_pEditor->setStatus(255, 0, 0, "Error: Could not read mode data.");
        return;
      }

      g_pEditor->setStatus(255, 255, 0, "Done reading mode " + to_string(i));

      Mode newMode(g_pEditor->m_engine);
      newMode.unserialize(modeBuffer);
      if (!g_pEditor->m_vortex.addMode(&newMode)) {
        g_pEditor->setStatus(255, 0, 0, "Failed to add mode " + to_string(i));
        return;
      }

      g_pEditor->setStatus(255, 255, 0, "Imported mode " + to_string(i));
    }

    string msg = "Pulled " + to_string(m_connectedDuo.numModes) + 
                      " modes from Duo v" + to_string(m_connectedDuo.vMajor) + 
                      "." + to_string(m_connectedDuo.vMinor);
    g_pEditor->setStatus(0, 255, 0, msg);
    g_pEditor->refreshModeList();
    g_pEditor->demoCurMode();
    g_pEditor->setStatus(0, 255, 0, "Connected");
  }).detach();
}

void VortexChromaLink::pushDuoModes()
{
  thread([this]() {
    // make sure there is a valid duo connected
    if (!m_connectedDuo.vMajor || !m_connectedPort) {
      g_pEditor->setStatus(255, 0, 0, "Connect a Duo first");
      return;
    }

    //g_pEditor->setStatus(255, 255, 0, "Writing header...");

    //m_connectedPort->writeData(EDITOR_VERB_PUSH_CHROMA_HDR);
    //m_connectedPort->expectData(EDITOR_VERB_READY);

    //// but change the number of modes
    //m_connectedDuo.numModes = g_pEditor->m_engine.modes().numModes();

    //ByteStream headerBuffer(sizeof(m_connectedDuo), (const uint8_t *)&m_connectedDuo);
    //m_connectedPort->writeData(headerBuffer);
    //m_connectedPort->expectData(EDITOR_VERB_PUSH_CHROMA_HDR_ACK);

    g_pEditor->setStatus(255, 255, 0, "Pushing modes to Duo...");

    uint8_t oldModeIdx = g_pEditor->m_engine.modes().curModeIndex();
    g_pEditor->m_engine.modes().setCurMode(0);

    for (uint8_t i = 0; i < m_connectedDuo.numModes; ++i) {
      ByteStream modeBuffer;
      g_pEditor->setStatus(255, 255, 0, "Writing mode " + to_string(i) + "...");
      m_connectedPort->writeData(EDITOR_VERB_PUSH_CHROMA_MODE);
      m_connectedPort->expectData(EDITOR_VERB_READY);

      g_pEditor->setStatus(255, 255, 0, "Writing mode idx " + to_string(i) + "...");
      m_connectedPort->writeData((uint8_t *)&i, 1);
      m_connectedPort->expectData(EDITOR_VERB_READY);

      Mode *cur = g_pEditor->m_engine.modes().curMode();
      if (!cur) {
        // fatal
        g_pEditor->setStatus(255, 0, 0, "Failed to iterate modes");
        return;
      }
      cur->serialize(modeBuffer);
      modeBuffer.recalcCRC();
      
      g_pEditor->setStatus(255, 255, 0, "Writing mode data " + to_string(i) + "...");
      m_connectedPort->writeData(modeBuffer);
      m_connectedPort->expectData(EDITOR_VERB_PUSH_CHROMA_MODE_ACK);

      // continue to next mode
      g_pEditor->m_engine.modes().nextMode();

      g_pEditor->setStatus(255, 255, 0, "Done writing mode " + to_string(i));
    }

    g_pEditor->m_engine.modes().setCurMode(oldModeIdx);

    string msg = "Pushed " + to_string(m_connectedDuo.numModes) + 
                      " modes to Duo v" + to_string(m_connectedDuo.vMajor) + 
                      "." + to_string(m_connectedDuo.vMinor);
    g_pEditor->setStatus(0, 255, 0, msg);
    g_pEditor->refreshModeList();
    g_pEditor->demoCurMode();
    g_pEditor->setStatus(0, 255, 0, "Connected");
  }).detach();
}

void VortexChromaLink::flashFirmware()
{
  thread([this]() {
    g_pEditor->setStatus(255, 255, 0, "Flashing Duo Firmware...");

    VortexPort *port = nullptr;
    if (!g_pEditor->isConnected() || !g_pEditor->getCurPort(&port)) {
      g_pEditor->setStatus(255, 0, 0, "No connection.");
      return;
    }

    OPENFILENAME filenameInfo = {};
    char filePath[MAX_PATH] = {};
    filenameInfo.lStructSize = sizeof(OPENFILENAME);
    filenameInfo.lpstrFile = filePath;
    filenameInfo.nMaxFile = MAX_PATH;
    filenameInfo.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    filenameInfo.lpstrFilter = "Firmware Files\0*.bin\0All Files\0*.*\0";

    if (!GetOpenFileName(&filenameInfo)) {
      g_pEditor->setStatus(255, 0, 0, "Failed to open firmware file.");
      return;
    }

    std::ifstream firmwareFile(filePath, std::ios::binary | std::ios::ate);
    if (!firmwareFile.is_open()) {
      g_pEditor->setStatus(255, 0, 0, "Could not open firmware file.");
      return;
    }

    uint32_t firmwareSize = (uint32_t)firmwareFile.tellg();
    firmwareFile.seekg(0, std::ios::beg);

    if (firmwareSize <= 0) {
      g_pEditor->setStatus(255, 0, 0, "Invalid firmware file.");
      return;
    }

    std::vector<char> buffer(firmwareSize);
    if (!firmwareFile.read(buffer.data(), firmwareSize)) {
      g_pEditor->setStatus(255, 0, 0, "Failed to read firmware file.");
      return;
    }

    firmwareFile.close();

    m_connectedPort->writeData(EDITOR_VERB_FLASH_FIRMWARE);
    if (!port->expectData(EDITOR_VERB_READY)) {
      g_pEditor->setStatus(255, 0, 0, "Failed to receive initial flash firmware ACK");
      return;
    }

    ByteStream sizeBuf;
    sizeBuf.serialize32(firmwareSize);
    sizeBuf.recalcCRC();
    // Send the firmware size
    port->writeData(sizeBuf);
    if (!port->expectData(EDITOR_VERB_READY)) {
      g_pEditor->setStatus(255, 0, 0, "Failed to receive flash firmware size ACK");
      return;
    }

    g_pEditor->setStatus(255, 255, 0, "Writing firmware...");

    auto start = std::chrono::high_resolution_clock::now();

    ////// Write the firmware data
    ////port->writeData((uint8_t *)buffer.data(), firmwareSize);
    ////port->expectData(EDITOR_VERB_FLASH_FIRMWARE_DONE);

   // Send firmware data in chunks
    const uint32_t chunkSize = FIRMWARE_TRANSFER_BLOCK_SIZE;
    uint32_t remainder = firmwareSize;
    uint32_t offset = 0;
    uint32_t chunk = 0;
    uint32_t total = (firmwareSize / chunkSize) + ((firmwareSize % chunkSize) != 0);
    while (remainder > 0) {
      // Update status for progress tracking
      string percent = std::to_string((int)((chunk * 100.0) / total)) + "%";
      g_pEditor->setStatus(255, 255, 0, "Writing firmware: " + std::to_string(chunk * chunkSize) + " / " + std::to_string(total * chunkSize) + " (" + percent + ")...");

      uint32_t bytesToSend = (remainder < chunkSize) ? remainder : chunkSize;
      ByteStream firmwareChunk(bytesToSend, (uint8_t *)buffer.data() + offset);

      // Write each chunk
      if (!port->writeData(firmwareChunk)) {
        g_pEditor->setStatus(255, 0, 0, "Failed to write firmware chunk: " + std::to_string(chunk) + " / " + std::to_string(total));
        return;
      }
      if (!port->expectData(EDITOR_VERB_FLASH_FIRMWARE_ACK)) {
        g_pEditor->setStatus(255, 0, 0, "Failed to receive ACK for firmware chunk: " + std::to_string(chunk) + " / " + std::to_string(total));
        return;
      }

      // adjust counters
      remainder -= bytesToSend;
      offset += bytesToSend;
      chunk += 1;
    }

    //if (!port->expectData(EDITOR_VERB_FLASH_FIRMWARE_DONE)) {
    //  g_pEditor->setStatus(255, 0, 0, "Failed to receive DONE for firmware flash");
    //  return;
    //}

    auto end = std::chrono::high_resolution_clock::now();

    g_pEditor->setStatus(255, 255, 0, "Firmware flashing completed.");

    //port->writeData(EDITOR_VERB_FLASH_FIRMWARE_DONE);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    int seconds = (int)duration.count() / 1000;
    int milliseconds = duration.count() % 1000;
    string statusStr = "Wrote firmware to Duo. Time taken: " + to_string(seconds) + "." + to_string(milliseconds) + " s";
    g_pEditor->setStatus(0, 255, 0, statusStr);
    g_pEditor->refreshModeList();
    g_pEditor->demoCurMode();
    //thread([this]() {
    //  // change the status 3 seconds later
    //  Sleep(3000);
    //  g_pEditor->setStatus(0, 255, 0, "Connected");
    //}).detach();
  }).detach();
}

void VortexChromaLink::pullSingleDuoMode()
{
  thread([this]() {
    // make sure there is a valid duo connected
    if (!m_connectedDuo.vMajor || !m_connectedPort) {
      g_pEditor->setStatus(255, 0, 0, "Connect a Duo first");
      return;
    }

    g_pEditor->setStatus(255, 255, 0, "Pulling modes from Duo...");

    uint8_t modeIdx = g_pEditor->m_engine.modes().curModeIndex();
    ByteStream modeBuffer;
    m_connectedPort->writeData(EDITOR_VERB_PULL_CHROMA_MODE);
    g_pEditor->setStatus(255, 255, 0, "Reading mode " + to_string(modeIdx) + "...");

    m_connectedPort->expectData(EDITOR_VERB_READY);
    m_connectedPort->writeData((uint8_t *)&modeIdx, 1);

    modeBuffer.clear();
    if (!m_connectedPort->readByteStream(modeBuffer) || !modeBuffer.size()) {
      g_pEditor->setStatus(255, 0, 0, "Error: Could not read mode data.");
      return;
    }

    Mode newMode(g_pEditor->m_engine);
    newMode.unserialize(modeBuffer);
    g_pEditor->m_vortex.addMode(&newMode);

    string msg = "Pulled mode " + to_string(modeIdx) + " from Duo v" +
      to_string(m_connectedDuo.vMajor) + "." + to_string(m_connectedDuo.vMinor);
    g_pEditor->setStatus(0, 255, 0, msg);
    g_pEditor->refreshModeList();
    g_pEditor->demoCurMode();
    g_pEditor->setStatus(0, 255, 0, "Connected");
  }).detach();
}

void VortexChromaLink::pushSingleDuoMode()
{
  thread([this]() {
    g_pEditor->setStatus(255, 255, 0, "Pushing modes to Duo...");

    VortexPort *port = nullptr;
    if (!g_pEditor->isConnected() || !g_pEditor->getCurPort(&port)) {
      g_pEditor->setStatus(255, 0, 0, "No connection.");
      return;
    }

    uint8_t modeIdx = g_pEditor->m_engine.modes().curModeIndex();

    g_pEditor->setStatus(255, 255, 0, "Writing modes...");

    ByteStream modeBuffer;
    m_connectedPort->writeData(EDITOR_VERB_PUSH_CHROMA_MODE);
    g_pEditor->setStatus(255, 255, 0, "Writing mode " + to_string(modeIdx) + "...");

    m_connectedPort->expectData(EDITOR_VERB_READY);
    m_connectedPort->writeData((uint8_t *)&modeIdx, 1);
    m_connectedPort->expectData(EDITOR_VERB_READY);

    Mode *cur = g_pEditor->m_engine.modes().curMode();
    if (cur) {
      cur->serialize(modeBuffer);
    }
    modeBuffer.recalcCRC();
    m_connectedPort->writeData(modeBuffer);
    m_connectedPort->expectData(EDITOR_VERB_PUSH_CHROMA_MODE_ACK);

    g_pEditor->setStatus(255, 255, 0, "Done writing mode " + to_string(modeIdx));

    string msg = "Pushed mode " + to_string(modeIdx) + 
                      " to Duo v" + to_string(m_connectedDuo.vMajor) + 
                      "." + to_string(m_connectedDuo.vMinor);
    g_pEditor->setStatus(0, 255, 0, msg);
    g_pEditor->refreshModeList();
    g_pEditor->demoCurMode();
    g_pEditor->setStatus(0, 255, 0, "Connected");
  }).detach();
}


