#include "VortexEditor.h"

// VortexEngine includes
#include "VortexEngine.h"
#include "Serial/ByteStream.h"
#include "Colors/Colorset.h"
#include "EditorConfig.h"
#include "Modes/Modes.h"
#include "Modes/Mode.h"
#include "Modes/ModeBuilder.h"

// for random()
#include "Arduino.h"

// Editor includes
#include "ArduinoSerial.h"
#include "GUI/VWindow.h"
#include "resource.h"

// stl includes
#include <string>

// for registering ui elements for events
#define SELECT_PORT_ID  50001
#define SELECT_MODE_ID  50002
#define ADD_MODE_ID     50003
#define DEL_MODE_ID     50004

using namespace std;

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
  m_hInstance(NULL),
  m_consoleHandle(nullptr),
  m_portList(),
  m_window(),
  m_connectButton(),
  m_pushButton(),
  m_pullButton(),
  m_loadButton(),
  m_saveButton(),
  m_portSelection(),
  m_modeListBox()
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

  if (!m_consoleHandle) {
    AllocConsole();
    freopen_s(&m_consoleHandle, "CONOUT$", "w", stdout);
  }

  // init the engine
  VortexEngine::init();
  // clear the modes
  Modes::clearModes();

  // initialize the window accordingly
  m_window.init(hInstance, EDITOR_TITLE, EDITOR_BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT, g_pEditor);
  m_portSelection.init(hInstance, m_window, "Select Port", EDITOR_BACK_COL, 150, 300, 16, 16, SELECT_PORT_ID, selectPortCallback);
  m_connectButton.init(hInstance, m_window, "Connect", EDITOR_BACK_COL, 72, 28, 16, 48, ID_FILE_CONNECT, connectCallback);
  m_pushButton.init(hInstance, m_window, "Push", EDITOR_BACK_COL, 72, 28, 16, 80, ID_FILE_PUSH, pushCallback);
  m_pullButton.init(hInstance, m_window, "Pull", EDITOR_BACK_COL, 72, 28, 16, 112, ID_FILE_PULL, pullCallback);
  m_loadButton.init(hInstance, m_window, "Load", EDITOR_BACK_COL, 72, 28, 16, 144, ID_FILE_LOAD, loadCallback);
  m_saveButton.init(hInstance, m_window, "Save", EDITOR_BACK_COL, 72, 28, 16, 176, ID_FILE_SAVE, saveCallback);
  m_modeListBox.init(hInstance, m_window, "Mode List", EDITOR_BACK_COL, 250, 300, 16, 210, SELECT_MODE_ID, selectModeCallback);
  m_addModeButton.init(hInstance, m_window, "Add", EDITOR_BACK_COL, 74, 28, 92, 503, ADD_MODE_ID, addModeCallback);
  m_delModeButton.init(hInstance, m_window, "Del", EDITOR_BACK_COL, 72, 28, 16, 503, DEL_MODE_ID, delModeCallback);

  // scan for any connections
  scanPorts();

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

void VortexEditor::readInLoop(uint32_t port, ByteStream &outStream)
{
  outStream.clear();
  while (1) {
    if (!readPort(port, outStream)) {
      // error?
      continue;
    }
    if (!outStream.size()) {
      continue;
    }
    break;
  }
}

void VortexEditor::connect()
{
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // try to read the handshake
  if (!readPort(port, stream)) {
    // failure
    return;
  }
  if (!validateHandshake(stream)) {
    // failure
    return;
  }
  writePort(port, EDITOR_VERB_HELLO_ACK);
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_IDLE) != 0) {
    // ???
  }
  // k
  writePort(port, EDITOR_VERB_IDLE_ACK);
}

bool VortexEditor::validateHandshake(const ByteStream &handshake)
{
  // check the handshake for valid data
  if (handshake.size() < 10) {
    printf("Handshake size bad: %u\n", handshake.size());
    // bad handshake
    return false;
  }
  if (handshake.data()[0] != '=' || handshake.data()[1] != '=') {
    printf("Handshake start bad: [%c%c]\n",
      handshake.data()[0], handshake.data()[1]);
    // bad handshake
    return false;
  }
  if (memcmp(handshake.data(), "== Vortex Framework v", sizeof("== Vortex Framework v") - 1) != 0) {
    printf("Handshake data bad: [%s]\n", handshake.data());
    // bad handshake
    return false;
  }
  // looks good
  return true;
}

void VortexEditor::refreshModeList()
{
  m_modeListBox.clearItems();
  int curSel = Modes::curModeIndex();
  Modes::setCurMode(0);
  for (uint32_t i = 0; i < Modes::numModes(); ++i) {
    Mode *curMode = Modes::curMode();
    if (!curMode) {
      // ?
      continue;
    }
    string modeName = "Mode " + to_string(i) + " (" + getPatternName(curMode->getPatternID()) + ")";
    m_modeListBox.addItem(modeName);
    // go to next mode
    Modes::nextMode();
  }
  // restore the selection
  m_modeListBox.setSelection(curSel);
  Modes::setCurMode(curSel);
  printf("Refreshed mode list\n");
}

void VortexEditor::push()
{
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_PUSH_MODES);
  // read data again
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_PUSH_MODES_RDY) != 0) {
    // ??
  }
  // now unserialize the stream of data that was read
  ByteStream modes;
  Modes::serialize(modes);
  // send the modes
  writePort(port, modes);
  // wait for the done response
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_PUSH_MODES_DONE) != 0) {
    // ??
  }
  // now wait for idle
  waitIdle();
}

void VortexEditor::pull()
{
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_PULL_MODES);
  stream.clear();
  if (!readModes(port, stream) || !stream.size()) {
    printf("Couldn't read anything\n");
    return;
  }
  if (!stream.checkCRC()) {
    printf("BAD CRC !\n");
    return;
  }
  // now unserialize the stream of data that was read
  if (!Modes::unserialize(stream)) {
    printf("Unserialize failed\n");
  }
  // now send the pull ack, thx bro
  writePort(port, EDITOR_VERB_PULL_MODES_ACK);
  // unserialized all our modes
  printf("Unserialized %u modes\n", Modes::numModes());
  // now wait for idle
  waitIdle();
  // refresh the mode list
  refreshModeList();
}

void VortexEditor::load()
{
  const char filename[] = "SaveFile.vortex";
  HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD bytesRead = 0;
  ByteStream stream(4096);
  if (!ReadFile(hFile, (void *)stream.rawData(), stream.capacity(), &bytesRead, NULL)) {
    // error
  }
  CloseHandle(hFile);
  stream.shrink();
  if (!stream.checkCRC()) {
    printf("Bad crc\n");
    return;
  }
  // load the modes
  Modes::unserialize(stream);
  printf("Loaded from [%s]\n", filename);
}

void VortexEditor::save()
{
  ByteStream stream;
  Modes::serialize(stream);
  const char filename[] = "SaveFile.vortex";
  HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  stream.recalcCRC();
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  printf("Saved to [%s]\n", filename);
}

void VortexEditor::selectPort()
{
}

void VortexEditor::selectMode()
{
  Modes::setCurMode(m_modeListBox.getSelection());
}

void VortexEditor::addMode()
{
  Colorset set;
  set.randomize();
    // create a random pattern ID from all patterns
  PatternID randomPattern;
  do {
    // continuously re-randomize the pattern so we don't get solids
    randomPattern = (PatternID)random(PATTERN_FIRST, PATTERN_COUNT);
  } while (randomPattern >= PATTERN_SOLID0 && randomPattern <= PATTERN_SOLID2);
  Modes::addMode(randomPattern, &set);
  refreshModeList();
}

void VortexEditor::delMode()
{
  Modes::deleteCurMode();
  refreshModeList();
}

void VortexEditor::waitIdle()
{
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // now wait for the idle again
  readInLoop(port, stream);
  // check for idle
  if (strcmp((char *)stream.data(), EDITOR_VERB_IDLE) != 0) {
    // ???
  }
  // send idle ack
  writePort(m_portSelection.getSelection(), EDITOR_VERB_IDLE_ACK);
}

void VortexEditor::scanPorts()
{
  for (uint32_t i = 0; i < 255; ++i) {
    string port = "\\\\.\\COM" + to_string(i);
    ArduinoSerial serialPort(port);
    if (serialPort.IsConnected()) {
      m_portList.push_back(make_pair(i, move(serialPort)));
    }
  }
  for (auto port = m_portList.begin(); port != m_portList.end(); ++port) {
    m_portSelection.addItem(to_string(port->first));
    printf("Connected port %u\n", port->first);
  }
}

bool VortexEditor::readPort(uint32_t portIndex, ByteStream &outStream)
{
  if (portIndex >= m_portList.size()) {
    return false;
  }
  ArduinoSerial *serial = &m_portList[portIndex].second;
  // read with NULL args to get expected amount
  int32_t amt = serial->ReadData(NULL, 0);
  if (amt == -1 || amt == 0) {
    // no data to read
    return false;
  }
  outStream.init(amt);
  // read the data into the buffer
  serial->ReadData((void *)outStream.data(), amt);
  // size is the first param of the data, just override it
  // idk I don't want to change the ByteStream class to accomodate
  // the editor, maybe the Serial class should accomodate the Bytestream
  *(uint32_t *)outStream.rawData() = amt;
  // just print the buffer
  printf("Data on port %u: [%s] (%u bytes)\n", m_portList[portIndex].first, outStream.data(), amt);
  return true;
}

bool VortexEditor::readModes(uint32_t portIndex, ByteStream &outModes)
{
  if (portIndex >= m_portList.size()) {
    return false;
  }
  ArduinoSerial *serial = &m_portList[portIndex].second;
  uint32_t size = 0;

  // first check how much is in the serial port
  int32_t amt = 0;
  do {
    // read with NULL args to get expected amount
    amt = serial->ReadData(NULL, 0);
    // we need at least a size value
  } while (amt < sizeof(size));

  // read the size out of the serial port
  serial->ReadData((void *)&size, sizeof(size));
  if (!size || size > 4096) {
    DEBUG_LOGF("Bad IR Data size: %u", size);
    return false;
  }

  // init outmodes so it's big enough
  outModes.init(size);
  uint32_t amtRead = 0;
  do {
    // read straight into the raw buffer, this will always have enough
    // space because outModes is big enough to hold the entire data
    uint8_t *readPos = ((uint8_t *)outModes.rawData()) + amtRead;
    amtRead += serial->ReadData((void *)readPos, size);
  } while (amtRead < size);
  return true;
}

void VortexEditor::writePortRaw(uint32_t portIndex, const uint8_t *data, size_t size)
{
  if (portIndex >= m_portList.size()) {
    return;
  }
  ArduinoSerial *serial = &m_portList[portIndex].second;
  // write the data into the serial port
  serial->WriteData(data, size);
}

void VortexEditor::writePort(uint32_t portIndex, const ByteStream &data)
{
  if (portIndex >= m_portList.size()) {
    return;
  }
  uint32_t size = data.rawSize();
  writePortRaw(portIndex, (uint8_t *)&size, sizeof(size));
  writePortRaw(portIndex, (uint8_t *)data.rawData(), size);
  printf("Wrote %u bytes of raw data\n", size);
}

void VortexEditor::writePort(uint32_t portIndex, string data)
{
  if (portIndex >= m_portList.size()) {
    return;
  }
  writePortRaw(portIndex, (uint8_t *)data.c_str(), data.size());
  // just print the buffer
  printf("Wrote to port %u: [%s]\n", m_portList[portIndex].first, data.c_str());
}

string VortexEditor::getPatternName(PatternID id) const
{
  if (id == PATTERN_NONE || id >= PATTERN_COUNT) {
    return "pattern_none";
  }
  static const char *patternNames[PATTERN_COUNT] = {
    "basic",
    "strobe",
    "hyperstrobe",
    "dops",
    "dopish",
    "ultradops",
    "strobie",
    "ribbon",
    "miniribbon",
    "tracer",
    "dashdops",
    "blinkie",
    "ghostcrush",
    "advanced",
    "blend",
    "complementary blend",
    "brackets",

    "solid0",
    "solid1",
    "solid2",

    "rabbit",
    "hueshift",
    "theater chase",
    "chaser",
    "zigzag",
    "zipfade",
    "tiptop",
    "drip",
    "dripmorph",
    "crossdops",
    "doublestrobe",
    // TODO: SLoth Strobe doubleStrobe(5, 8, 1000)
    "meteor",
    "sparkletrace",
    "vortexwipe",
    // TODO: UltraWipe vortexWipe(2, 7, 50)
    "warp",
    "warpworm",
    "snowball",
    "lighthouse",
    "pulsish",
    "fill",
    "bounce",
    "impact",
    "splitstrobie",
    "backstrobe",
    "flowers",
    "jest",
    "materia"
  };
  return patternNames[id];
}

void VortexEditor::printlog(const char *file, const char *func, int line, const char *msg, va_list list)
{
  string strMsg;
  if (file) {
    strMsg = file;
    if (strMsg.find_last_of('\\') != string::npos) {
      strMsg = strMsg.substr(strMsg.find_last_of('\\') + 1);
    }
    strMsg += ":";
    strMsg += to_string(line);
  }
  if (func) {
    strMsg += " ";
    strMsg += func;
    strMsg += "(): ";
  }
  strMsg += msg;
  strMsg += "\n";
  vfprintf(g_pEditor->m_consoleHandle, strMsg.c_str(), list);
  //vfprintf(g_pEditor->m_logHandle, strMsg.c_str(), list);
}
