#include "VortexEditor.h"

// VortexEngine includes
#include "Serial/ByteStream.h"
#include "Colors/Colorset.h"
#include "Patterns/Pattern.h"

// Editor includes
#include "EngineWrapper.h"
#include "ArduinoSerial.h"
#include "EditorConfig.h"
#include "GUI/VWindow.h"
#include "resource.h"

// stl includes
#include <string>

// for registering ui elements for events
#define SELECT_PORT_ID      50001
#define SELECT_MODE_ID      50002
#define ADD_MODE_ID         50003
#define DEL_MODE_ID         50004
#define SELECT_FINGER_ID    50005
#define SELECT_PATTERN_ID   50006
#define SELECT_COLOR_ID     50007
#define PARAM_EDIT_ID       50016

using namespace std;

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
  m_hInstance(NULL),
  m_consoleHandle(nullptr),
  m_portList(),
  m_window(),
  m_portSelection(),
  m_connectButton(),
  m_pushButton(),
  m_pullButton(),
  m_loadButton(),
  m_saveButton(),
  m_modeListBox(),
  m_addModeButton(),
  m_delModeButton(),
  m_fingersListBox(),
  m_patternSelectComboBox(),
  m_colorSelects()
{
}

VortexEditor::~VortexEditor()
{
}

bool VortexEditor::init(HINSTANCE hInst)
{
  if (g_pEditor) {
    return false;
  }
  g_pEditor = this;

  m_hInstance = hInst;

  if (!m_consoleHandle) {
    AllocConsole();
    freopen_s(&m_consoleHandle, "CONOUT$", "w", stdout);
  }

  // initialize the system that wraps the vortex engine
  VEngine::init();

  // initialize the window accordingly
  m_window.init(hInst, EDITOR_TITLE, BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT, g_pEditor);
  m_portSelection.init(hInst, m_window, "Select Port", BACK_COL, 150, 300, 16, 16, SELECT_PORT_ID, selectPortCallback);
  m_refreshButton.init(hInst, m_window, "Refresh", BACK_COL, 72, 25, 180, 16, ID_FILE_REFRESH, refreshCallback);
  m_connectButton.init(hInst, m_window, "Connect", BACK_COL, 72, 28, 16, 48, ID_FILE_CONNECT, connectCallback);
  m_pushButton.init(hInst, m_window, "Push", BACK_COL, 72, 28, 16, 80, ID_FILE_PUSH, pushCallback);
  m_pullButton.init(hInst, m_window, "Pull", BACK_COL, 72, 28, 16, 112, ID_FILE_PULL, pullCallback);
  m_loadButton.init(hInst, m_window, "Load", BACK_COL, 72, 28, 16, 144, ID_FILE_LOAD, loadCallback);
  m_saveButton.init(hInst, m_window, "Save", BACK_COL, 72, 28, 16, 176, ID_FILE_SAVE, saveCallback);
  m_modeListBox.init(hInst, m_window, "Mode List", BACK_COL, 250, 300, 16, 210, SELECT_MODE_ID, selectModeCallback);
  m_addModeButton.init(hInst, m_window, "Add", BACK_COL, 74, 28, 92, 503, ADD_MODE_ID, addModeCallback);
  m_delModeButton.init(hInst, m_window, "Del", BACK_COL, 72, 28, 16, 503, DEL_MODE_ID, delModeCallback);
  m_fingersListBox.init(hInst, m_window, "Fingers", BACK_COL, 180, 300, 288, 210, SELECT_FINGER_ID, selectFingerCallback);
  m_patternSelectComboBox.init(hInst, m_window, "Select Pattern", BACK_COL, 150, 300, 490, 210, SELECT_PATTERN_ID, selectPatternCallback);

  for (uint32_t i = 0; i < 8; ++i) {
    m_colorSelects[i].init(hInst, m_window, "Color Select", BACK_COL, 36, 30, 490, 240 + (33 * i), SELECT_COLOR_ID + i, selectColorCallback);
  }

  for (uint32_t i = 0; i < 8; ++i) {
    m_paramTextBoxes[i].init(hInst, m_window, "", BACK_COL, 64, 24, 670, 210 + (32 * i), PARAM_EDIT_ID + i, paramEditCallback);
  }

  // trigger a refresh
  refreshModeList();

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

void VortexEditor::selectPort(VWindow *window)
{
  // connect to port?
}

// refresh the port list
void VortexEditor::refresh(VWindow *window)
{
  if (!window) {
    return;
  }
  m_portList.clear();
  for (uint32_t i = 0; i < 255; ++i) {
    string port = "\\\\.\\COM" + to_string(i);
    ArduinoSerial serialPort(port);
    if (serialPort.IsConnected()) {
      m_portList.push_back(make_pair(i, move(serialPort)));
    }
  }
  m_portSelection.clearItems();
  for (auto port = m_portList.begin(); port != m_portList.end(); ++port) {
    m_portSelection.addItem("Port " + to_string(port->first));
    printf("Connected port %u\n", port->first);
  }
}

void VortexEditor::connect(VWindow *window)
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

void VortexEditor::push(VWindow *window)
{
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_PUSH_MODES);
  // read data again
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_READY) != 0) {
    // ??
  }
  // now unserialize the stream of data that was read
  ByteStream modes;
  VEngine::getModes(modes);
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

void VortexEditor::pull(VWindow *window)
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
  VEngine::setModes(stream);
  // now send the pull ack, thx bro
  writePort(port, EDITOR_VERB_PULL_MODES_ACK);
  // unserialized all our modes
  printf("Unserialized %u modes\n", VEngine::numModes());
  // now wait for idle
  waitIdle();
  // refresh the mode list
  refreshModeList();
}

void VortexEditor::load(VWindow *window)
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
  VEngine::setModes(stream);
  printf("Loaded from [%s]\n", filename);
  refreshModeList();
}

void VortexEditor::save(VWindow *window)
{
  const char filename[] = "SaveFile.vortex";
  HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  ByteStream stream;
  VEngine::getModes(stream);
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  printf("Saved to [%s]\n", filename);
}

void VortexEditor::selectMode(VWindow *window)
{
  int sel = m_modeListBox.getSelection();
  if (sel < 0) {
    return;
  }
  VEngine::setCurMode(sel);
  // reselect first finger
  m_fingersListBox.setSelection(0);
  refreshFingerList();
  demoCurMode();
}

void VortexEditor::demoCurMode()
{
  if (!isConnected()) {
    return;
  }
  ByteStream stream;
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_DEMO_MODE);
  // read data again
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_READY) != 0) {
    // ??
  }
  // now unserialize the stream of data that was read
  ByteStream curMode;
  VEngine::getCurMode(curMode);
  // send the mode
  writePort(port, curMode);
  // wait for the done response
  readInLoop(port, stream);
  if (strcmp((char *)stream.data(), EDITOR_VERB_DEMO_MODE_DONE) != 0) {
    // ??
  }
  // now wait for idle
  waitIdle();
}

void VortexEditor::addMode(VWindow *window)
{
  if (VEngine::numModes() >= MAX_MODES) {
    return;
  }
  printf("Adding mode %u\n", VEngine::numModes() + 1);
  VEngine::addNewMode();
  refreshModeList();
}

void VortexEditor::delMode(VWindow *window)
{
  printf("Deleting mode %u\n", VEngine::curMode());
  VEngine::delCurMode();
  refreshModeList();
}

void VortexEditor::selectFinger(VWindow *window)
{
  refreshPatternSelect();
  refreshColorSelect();
  refreshParams();
}

void VortexEditor::selectPattern(VWindow *window)
{
  int pat = m_patternSelectComboBox.getSelection();
  if (pat < 0) {
    return;
  }
  int pos = m_fingersListBox.getSelection();
  if (pos < 0) {
    return;
  }
  if (pos == 0) {
    // set the pattern on the entire mode
    VEngine::setPattern((PatternID)pat);
  } else {
    // only set the pattern on a single position
    VEngine::setSinglePat((LedPos)pos, (PatternID)pat);
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::selectColor(VWindow *window)
{
  if (!window) {
    return;
  }
  VColorSelect *colSelect = (VColorSelect *)window;
  int pos = m_fingersListBox.getSelection();
  if (pos < 0) {
    return;
  }
  uint32_t colorIndex = (uint32_t)((uintptr_t)window->menu() - SELECT_COLOR_ID);
  Colorset newSet;
  VEngine::getColorset((LedPos)pos, newSet);
  // if the color select was made inactive
  if (!colSelect->isActive()) {
    printf("Disabled color slot %u\n", colorIndex);
    newSet.removeColor(colorIndex);
  } else {
    printf("Updating color slot %u\n", colorIndex);
    newSet.set(colorIndex, colSelect->getColor()); // getRawColor?
  }
  VEngine::setColorset((LedPos)pos, newSet);
  refreshColorSelect();
  // update the demo
  demoCurMode();
}

void VortexEditor::paramEdit(VWindow *window)
{
  if (!window || !window->isEnabled()) {
    return;
  }
  int sel = m_patternSelectComboBox.getSelection();
  if (sel < 0) {
    sel = 0;
  }
  int pos = m_fingersListBox.getSelection();
  if (pos < 0) {
    return;
  }
  uint32_t paramIndex = (uint32_t)((uintptr_t)window->menu() - PARAM_EDIT_ID);
  PatternArgs args;
  VEngine::getPatternArgs((LedPos)pos, args);
  // get the number of params for the current pattern selection
  uint32_t numParams = VEngine::numCustomParams((PatternID)sel);
  // store the target param
  args.args[paramIndex] = m_paramTextBoxes[paramIndex].getValue();
  VEngine::setPatternArgs((LedPos)pos, args);
  // update the demo
  demoCurMode();
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
  int curSel = VEngine::curMode();
  VEngine::setCurMode(0);
  for (uint32_t i = 0; i < VEngine::numModes(); ++i) {
    // just use the pattern name from the first pattern
    string modeName = "Mode " + to_string(i) + " (" + VEngine::getPatternName() + ")";
    m_modeListBox.addItem(modeName);
    // go to next mode
    VEngine::nextMode();
  }
  // restore the selection
  m_modeListBox.setSelection(curSel);
  VEngine::setCurMode(curSel);
  refreshFingerList();
}

void VortexEditor::refreshFingerList()
{
  int curSel = m_fingersListBox.getSelection();
  if (curSel < 0) {
    curSel = 0;
  }
  m_fingersListBox.clearItems();
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
    // if a finger is empty don't add it
    if (VEngine::getPatternID(pos) == PATTERN_NONE) {
      continue;
    }
    string fingerName = VEngine::ledToString(pos) + " (" + VEngine::getPatternName(pos) + ")";
    m_fingersListBox.addItem(fingerName);
  }
  // restore the selection
  m_fingersListBox.setSelection(curSel);
  refreshPatternSelect();
  refreshColorSelect();
  refreshParams();
}

void VortexEditor::refreshPatternSelect()
{
  int sel = m_fingersListBox.getSelection();
  if (sel < 0) {
    m_patternSelectComboBox.setSelection(PATTERN_NONE);
    return;
  }
  m_patternSelectComboBox.clearItems();
  bool allow_multi = (sel == 0);
  // get the pattern
  for (PatternID id = PATTERN_FIRST; id < PATTERN_COUNT; ++id) {
    if (!allow_multi && isMultiLedPatternID(id)) {
      continue;
    }
    m_patternSelectComboBox.addItem(VEngine::patternToString(id));
    if (id == VEngine::getPatternID((LedPos)sel)) {
      m_patternSelectComboBox.setSelection(id);
    }
  }
}

void VortexEditor::refreshColorSelect()
{
  int pos = m_fingersListBox.getSelection();
  if (pos < 0) {
    // iterate all extra slots and set to inactive
    for (uint32_t i = 0; i < 8; ++i) {
      m_colorSelects[i].clear();
      m_colorSelects[i].setActive(false);
    }
    return;
  }
  // get the colorset
  Colorset set;
  VEngine::getColorset((LedPos)pos, set);
  // iterate all active colors and set them
  for (uint32_t i = 0; i < set.numColors(); ++i) {
    m_colorSelects[i].setColor(set.get(i).raw());
    m_colorSelects[i].setActive(true);
  }
  // iterate all extra slots and set to inactive
  for (uint32_t i = set.numColors(); i < 8; ++i) {
    m_colorSelects[i].clear();
    m_colorSelects[i].setActive(false);
  }
}

void VortexEditor::refreshParams()
{
  int sel = m_patternSelectComboBox.getSelection();
  if (sel < 0) {
    sel = 0;
  }
  int pos = m_fingersListBox.getSelection();
  if (pos < 0) {
    for (uint32_t i = 0; i < 8; ++i) {
      m_paramTextBoxes[i].clearText();
      m_paramTextBoxes[i].setEnabled(false);
      m_paramTextBoxes[i].setVisible(false);
    }
    return;
  }
  PatternArgs args;
  VEngine::getPatternArgs((LedPos)pos, args);
  uint8_t *pArgs = (uint8_t *)&args.arg1;
  // get the number of params for the current pattern selection
  uint32_t numParams = VEngine::numCustomParams((PatternID)sel);
  // iterate all active params and activate
  for (uint32_t i = 0; i < numParams; ++i) {
    m_paramTextBoxes[i].setText(to_string(pArgs[i]).c_str());
    m_paramTextBoxes[i].setEnabled(true);
    m_paramTextBoxes[i].setVisible(true);
  }
  // iterate all extra slots and set to inactive
  for (uint32_t i = numParams; i < 8; ++i) {
    m_paramTextBoxes[i].clearText();
    m_paramTextBoxes[i].setEnabled(false);
    m_paramTextBoxes[i].setVisible(false);
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

void VortexEditor::readInLoop(uint32_t port, ByteStream &outStream)
{
  outStream.clear();
  // TODO: proper timeout lol
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

void VortexEditor::writePortRaw(uint32_t portIndex, const uint8_t *data, size_t size)
{
  if (portIndex >= m_portList.size()) {
    return;
  }
  ArduinoSerial *serial = &m_portList[portIndex].second;
  // write the data into the serial port
  serial->WriteData(data, (unsigned int)size);
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

bool VortexEditor::isConnected() const
{
  int sel = m_portSelection.getSelection();
  if (sel < 0) {
    return false;
  }
  if (!m_portList.size()) {
    return false;
  }
  return m_portList[sel].second.IsConnected();
}
