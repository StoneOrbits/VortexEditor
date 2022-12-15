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
#include <algorithm>
#include <sstream>
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
#define COPY_TO_ALL_ID      50025
#define COPY_MODE_ID        50026

// the prefix of colorsets copied to clipboard
#define COLORSET_CLIPBOARD_MARKER "COLORSET:"

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
  m_fingersMultiListBox(),
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

#ifdef _DEBUG
  if (!m_consoleHandle) {
    AllocConsole();
    freopen_s(&m_consoleHandle, "CONOUT$", "w", stdout);
  }
#endif

  // initialize the system that wraps the vortex engine
  VEngine::init();

  // initialize the window accordingly
  m_window.init(hInst, EDITOR_TITLE, BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT, g_pEditor);
  m_portSelection.init(hInst, m_window, "Select Port", BACK_COL, 80, 100, 16, 15, SELECT_PORT_ID, selectPortCallback);
  m_refreshButton.init(hInst, m_window, "Refresh", BACK_COL, 80, 24, 108, 15, ID_FILE_REFRESH_CONNECTIONS, refreshCallback);
  m_connectButton.init(hInst, m_window, "Connect", BACK_COL, 80, 24, 196, 15, ID_FILE_CONNECT, connectCallback);
  m_pushButton.init(hInst, m_window, "Push", BACK_COL, 80, 24, 284, 15, ID_FILE_PUSH, pushCallback);
  m_pullButton.init(hInst, m_window, "Pull", BACK_COL, 80, 24, 372, 15, ID_FILE_PULL, pullCallback);
  m_loadButton.init(hInst, m_window, "Load", BACK_COL, 80, 24, 460, 15, ID_FILE_LOAD, loadCallback);
  m_saveButton.init(hInst, m_window, "Save", BACK_COL, 80, 24, 548, 15, ID_FILE_SAVE, saveCallback);
  m_importButton.init(hInst, m_window, "Import", BACK_COL, 80, 24, 638, 15, ID_FILE_IMPORT, importCallback);
  m_exportButton.init(hInst, m_window, "Export", BACK_COL, 80, 24, 728, 15, ID_FILE_EXPORT, exportCallback);

  m_modeListBox.init(hInst, m_window, "Mode List", BACK_COL, 250, 270, 16, 54, SELECT_MODE_ID, selectModeCallback);

  m_addModeButton.init(hInst, m_window, "Add", BACK_COL, 80, 24, 101, 320, ADD_MODE_ID, addModeCallback);
  m_delModeButton.init(hInst, m_window, "Del", BACK_COL, 80, 24, 16, 320, DEL_MODE_ID, delModeCallback);
  m_copyModeButton.init(hInst, m_window, "Copy", BACK_COL, 80, 24, 185, 320, COPY_MODE_ID, copyModeCallback);

  m_fingersMultiListBox.init(hInst, m_window, "Fingers", BACK_COL, 230, 305, 278, 54, SELECT_FINGER_ID, selectFingerCallback);
  m_patternSelectComboBox.init(hInst, m_window, "Select Pattern", BACK_COL, 170, 300, 520, 54, SELECT_PATTERN_ID, selectPatternCallback);
  m_applyToAllButton.init(hInst, m_window, "Copy To All", BACK_COL, 108, 24, 700, 54, COPY_TO_ALL_ID, copyToAllCallback);

  for (uint32_t i = 0; i < 8; ++i) {
    m_colorSelects[i].init(hInst, m_window, "Color Select", BACK_COL, 36, 30, 520, 83 + (33 * i), SELECT_COLOR_ID + i, selectColorCallback);
  }

  for (uint32_t i = 0; i < 8; ++i) {
    m_paramTextBoxes[i].init(hInst, m_window, "", BACK_COL, 64, 24, 700, 86 + (32 * i), PARAM_EDIT_ID + i, paramEditCallback);
  }
  m_modeListBox.setTooltip("Test");

  // callbacks for menus
  m_window.addCallback(ID_COLORSET_RANDOM_COMPLIMENTARY, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_MONOCHROMATIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_TRIADIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_SQUARE, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_PENTADIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_RAINBOW, handleMenusCallback);
  m_window.addCallback(ID_PATTERN_RANDOM_SINGLE_LED_PATTERN, handleMenusCallback);
  m_window.addCallback(ID_PATTERN_RANDOM_MULTI_LED_PATTERN, handleMenusCallback);
  m_window.addCallback(ID_EDIT_CLEAR_COLORSET, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_COLOR_SET_TO_ALL, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_PATTERN_TO_ALL, handleMenusCallback);
  m_window.addCallback(ID_HELP_ABOUT, handleMenusCallback);
  m_window.addCallback(ID_HELP_HELP, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_COLORSET, handleMenusCallback);
  m_window.addCallback(ID_EDIT_PASTE_COLORSET, handleMenusCallback);

  // trigger a refresh
  refreshModeList();

  // apply the icon
  HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_window.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

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
  if (!g_pEditor || !g_pEditor->m_consoleHandle) {
    return;
  }
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

void VortexEditor::handleMenus(uintptr_t hMenu)
{
  switch (hMenu) {
  case ID_HELP_ABOUT:
    break;
  case ID_HELP_HELP:
    if (MessageBox(m_window.hwnd(), "It seems you need help", "Help", 4) == IDYES) {
      if (MessageBox(m_window.hwnd(), "Goodluck", "Help", 0)) {
      }
    }
    return;
  case ID_EDIT_COPY_COLORSET:
    copyColorset();
    return;
  case ID_EDIT_PASTE_COLORSET:
    pasteColorset();
    return;
  default:
    break;
  }
  uintptr_t menu = (uintptr_t)hMenu;
  int pos = m_fingersMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  Colorset newSet;
  switch (menu) {
  case ID_COLORSET_RANDOM_COMPLIMENTARY:
    newSet.randomizeComplimentary();
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_MONOCHROMATIC:
    newSet.randomizeMonochromatic();
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_TRIADIC:
    newSet.randomizeTriadic();
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_SQUARE:
    newSet.randomizeSquare();
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_PENTADIC:
    newSet.randomizePentadic();
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_RAINBOW:
    newSet.randomizeRainbow();
    applyColorset(newSet, sels);
    break;
  case ID_PATTERN_RANDOM_SINGLE_LED_PATTERN:
    // when applying a single led pattern we must use the 'setPattern' api
    // to properly convert the mode to all-same-single if it's a multi to
    // begin with, otherwise we can just apply the single to whichever we select
    if (isMultiLedPatternID(VEngine::getPatternID())) {
      VEngine::setPattern((PatternID)(rand() % PATTERN_SINGLE_COUNT));
    } else {
      applyPattern((PatternID)(rand() % PATTERN_SINGLE_COUNT), sels);
    }
    refreshModeList();
    demoCurMode();
    break;
  case ID_PATTERN_RANDOM_MULTI_LED_PATTERN:
    VEngine::setPattern((PatternID)((rand() % PATTERN_MULTI_COUNT) + PATTERN_MULTI_FIRST));
    refreshModeList();
    demoCurMode();
    break;
  case ID_EDIT_CLEAR_COLORSET:
    newSet.clear();
    applyColorset(newSet, sels);
    break;
  case ID_EDIT_COPY_COLOR_SET_TO_ALL:
    if (sels.size() != 1) {
      break;
    }
    VEngine::getColorset((LedPos)sels[0], newSet);
    applyColorsetToAll(newSet);
    break;
  case ID_EDIT_COPY_PATTERN_TO_ALL:
    if (sels.size() != 1) {
      break;
    }
    applyPatternToAll(VEngine::getPatternID((LedPos)sels[0]));
    break;
  }
}

void VortexEditor::applyColorset(const Colorset &set, const vector<int> &selections)
{
  for (uint32_t i = 0; i < selections.size(); ++i) {
    VEngine::setColorset((LedPos)selections[i], set);
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyPattern(PatternID id, const std::vector<int> &selections)
{
  for (uint32_t i = 0; i < selections.size(); ++i) {
    VEngine::setSinglePat((LedPos)selections[i], id);
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyColorsetToAll(const Colorset &set)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    VEngine::setColorset(i, set);
  }
  refreshColorSelect();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyPatternToAll(PatternID id)
{
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    VEngine::setSinglePat(i, id);
  }
  refreshFingerList();
  // update the demo
  demoCurMode();
}

void VortexEditor::copyColorset()
{
  string colorset = COLORSET_CLIPBOARD_MARKER;
  for (uint32_t i = 0; i < 8; ++i) {
    if (!m_colorSelects[i].isActive()) {
      break;
    }
    string name = m_colorSelects[i].getColorName();
    if (i > 0) {
      colorset += ",";
    }
    colorset += name;
  }
  setClipboard(colorset);
}

void VortexEditor::pasteColorset()
{
  string colorset;
  getClipboard(colorset);
  // check for the colorset marker
  if (strncmp(colorset.c_str(), COLORSET_CLIPBOARD_MARKER, sizeof(COLORSET_CLIPBOARD_MARKER) - 1) != 0) {
    return;
  }
  vector<std::string> splits;
  string split;
  istringstream ss(colorset.c_str() + sizeof(COLORSET_CLIPBOARD_MARKER) - 1);
  while (getline(ss, split, ',')) {
    splits.push_back(split);
  }
  Colorset newSet;
  for (auto field : splits) {
    if (field == "blank" || field[0] != '#') {
      newSet.addColor(0);
    } else {
      newSet.addColor(strtoul(field.c_str() + 1, NULL, 16));
    }
  }
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  for (uint32_t i = 0; i < sels.size(); ++i) {
    VEngine::setColorset((LedPos)sels[i], newSet);
  }
  refreshColorSelect();
  demoCurMode();
}

void VortexEditor::getClipboard(std::string &clipData)
{
  // Try opening the clipboard
  if (!OpenClipboard(nullptr)) {
    return;
  }
  // Get handle of clipboard object for ANSI text
  HANDLE hData = GetClipboardData(CF_TEXT);
  if (!hData) {
    return;
  }
  // Lock the handle to get the actual text pointer
  char *pszText = static_cast<char *>(GlobalLock(hData));
  if (!pszText) {
    GlobalUnlock(hData);
    CloseClipboard();
    return;
  }
  clipData = pszText;
  GlobalUnlock(hData);
  CloseClipboard();
}

void VortexEditor::setClipboard(const std::string &clipData)
{
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, clipData.length() + 1);
  if (!hMem) {
    // ??
    return;
  }
  void *data = GlobalLock(hMem);
  if (!data) {
    return;
  }
  memcpy(data, clipData.c_str(), clipData.length() + 1);
  GlobalUnlock(hMem);
  OpenClipboard(0);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, hMem);
  CloseClipboard();
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
  ArduinoSerial serialPort("\\\\.\\pipe\\vortextestframework");
  if (serialPort.IsConnected()) {
    m_portList.push_back(make_pair(0, move(serialPort)));
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
  int port = m_portSelection.getSelection();
  if (port < 0) {
    return;
  }
  // try to read the handshake
  if (!readPort(port, stream)) {
    // failure
    return;
  }
  if (!validateHandshake(stream)) {
    // failure
    return;
  }
  writePort(port, EDITOR_VERB_HELLO);
  // now wait for the idle again
  if (!expectData(port, EDITOR_VERB_HELLO_ACK)) {
    return;
  }
  m_portActive[port] = true;
}

void VortexEditor::push(VWindow *window)
{
  if (!isConnected()) {
    return;
  }
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_PUSH_MODES);
  // read data again
  expectData(port, EDITOR_VERB_READY);
  // now unserialize the stream of data that was read
  ByteStream modes;
  VEngine::getModes(modes);
  // send the modes
  writePort(port, modes);
  // wait for the done response
  expectData(port, EDITOR_VERB_PUSH_MODES_ACK);
}

void VortexEditor::pull(VWindow *window)
{
  if (!isConnected()) {
    return;
  }
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
  // now send the done message
  writePort(port, EDITOR_VERB_PULL_MODES_DONE);
  // wait for the ack from the gloves
  expectData(port, EDITOR_VERB_PULL_MODES_ACK);
  // unserialized all our modes
  printf("Unserialized %u modes\n", VEngine::numModes());
  // refresh the mode list
  refreshModeList();
  // demo the current mode
  demoCurMode();
}

void VortexEditor::load(VWindow *window)
{
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_pEditor->m_window.hwnd();
  char szFile[MAX_PATH] = {0};
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Save\0*.vortex\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  if (!GetOpenFileName(&ofn)) {
    return;
  }
  HANDLE hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
  if (!stream.decompress()) {
    // error
  }
  VEngine::setModes(stream);
  printf("Loaded from [%s]\n", szFile);
  refreshModeList();
}

void VortexEditor::save(VWindow *window)
{
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  char szFile[MAX_PATH] = "ModesBackup.vortex";
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Save\0*.vortex\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST;
  if (!GetSaveFileName(&ofn)) {
    return;
  }
  HANDLE hFile = CreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  ByteStream stream;
  VEngine::getModes(stream);
  if (!stream.compress()) {
    // error
  }
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  printf("Saved to [%s]\n", szFile);
  refreshModeList();
}

void VortexEditor::importMode(VWindow *window)
{
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_pEditor->m_window.hwnd();
  char szFile[MAX_PATH] = {0};
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Mode\0*.vtxmode\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  if (!GetOpenFileName(&ofn)) {
    return;
  }
  HANDLE hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
  if (!stream.decompress()) {
    // error
    return;
  }
  if (!VEngine::addNewMode(stream)) {
    // error
  }
  printf("Loaded from [%s]\n", szFile);
  refreshModeList();
}

void VortexEditor::exportMode(VWindow *window)
{
  if (!VEngine::numModes()) {
    return;
  }
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  string modeName = "Mode_" + to_string(VEngine::curMode()) + "_" + VEngine::getModeName();
  replace(modeName.begin(), modeName.end(), ' ', '_');
  modeName += ".vtxmode";
  char szFile[MAX_PATH] = {0};
  memcpy(szFile, modeName.c_str(), modeName.length());
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Mode\0*.vtxmode\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST;
  if (!GetSaveFileName(&ofn)) {
    return;
  }
  HANDLE hFile = CreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  ByteStream stream;
  VEngine::getCurMode(stream);
  if (!stream.compress()) {
    CloseHandle(hFile);
    // error
    return;
  }
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  printf("Saved to [%s]\n", szFile);
}

void VortexEditor::selectMode(VWindow *window)
{
  int sel = m_modeListBox.getSelection();
  if (sel < 0 || sel == VEngine::curMode()) {
    return;
  }
  VEngine::setCurMode(sel);
  // reselect first finger
  m_fingersMultiListBox.clearSelections();
  m_fingersMultiListBox.setSelection(0);
  refreshFingerList();
  demoCurMode();
}

void VortexEditor::demoCurMode()
{
  if (!isConnected()) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0 || !isConnected()) {
    return;
  }
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_DEMO_MODE);
  // read data again
  expectData(port, EDITOR_VERB_READY);
  // now unserialize the stream of data that was read
  ByteStream curMode;
  if (!VEngine::getCurMode(curMode) || !curMode.size()) {
    // error!
    // TODO: abort
  }
  // send, the, mode
  writePort(port, curMode);
  // wait for the done response
  expectData(port, EDITOR_VERB_DEMO_MODE_ACK);
}

void VortexEditor::clearDemo()
{
  if (!isConnected()) {
    return;
  }
  uint32_t port = m_portSelection.getSelection();
  // now immediately tell it what to do
  writePort(port, EDITOR_VERB_CLEAR_DEMO);
  // read data again
  expectData(port, EDITOR_VERB_CLEAR_DEMO_ACK);
}

void VortexEditor::addMode(VWindow *window)
{
  if (VEngine::numModes() >= MAX_MODES) {
    return;
  }
  printf("Adding mode %u\n", VEngine::numModes() + 1);
  VEngine::addNewMode();
  m_modeListBox.setSelection(VEngine::curMode());
  refreshModeList();
  if (VEngine::numModes() == 1) {
    m_fingersMultiListBox.setSelection(0);
    refreshModeList();
    demoCurMode();
  }
}

void VortexEditor::delMode(VWindow *window)
{
  printf("Deleting mode %u\n", VEngine::curMode());
  uint32_t cur = VEngine::curMode();
  VEngine::delCurMode();
  refreshModeList();
  if (!VEngine::numModes()) {
    clearDemo();
  } else {
    demoCurMode();
  }
}

void VortexEditor::copyMode(VWindow *window)
{
  if (!VEngine::numModes()) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0) {
    return;
  }
  printf("Copying mode %u\n", VEngine::curMode());
  ByteStream stream;
  VEngine::getCurMode(stream);
  VEngine::addNewMode(stream);
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
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  // if we ONLY selected the first led
  if (sels.size() == 1 && sels[0] == 0) {
    // and if we are switching from a multi-led or to a multi-led
    if (isMultiLedPatternID(VEngine::getPatternID()) ||
        isMultiLedPatternID((PatternID)pat)) {
      // then set the pattern on the entire mode
      VEngine::setPattern((PatternID)pat);
    } else {
      // otherwise we are switching from single to single to just
      // apply the pattern change to this slot
      VEngine::setSinglePat(LED_FIRST, (PatternID)pat);
    }
  } else {
    for (uint32_t i = 0; i < sels.size(); ++i) {
      // only set the pattern on a single position
      VEngine::setSinglePat((LedPos)sels[i], (PatternID)pat);
    }
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::copyToAll(VWindow *window)
{
  int pat = m_patternSelectComboBox.getSelection();
  if (pat < 0) {
    return;
  }
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (sels.size() > 1) {
    return;
  }
  int pos = m_fingersMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  if (isMultiLedPatternID((PatternID)pat)) {
    return;
  }
  PatternArgs args;
  VEngine::getPatternArgs((LedPos)pos, args);
  Colorset set;
  VEngine::getColorset((LedPos)pos, set);
  for (LedPos i = LED_FIRST; i < LED_COUNT; ++i) {
    if (pos == i) {
      continue;
    }
    VEngine::setSinglePat(i, (PatternID)pat, &args, &set);
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
  int pos = m_fingersMultiListBox.getSelection();
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
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  // set the colorset on all selected patterns
  for (uint32_t i = 0; i < sels.size(); ++i) {
    // only set the pattern on a single position
    VEngine::setColorset((LedPos)sels[i], newSet);
  }
  refreshModeList();
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
  int pos = m_fingersMultiListBox.getSelection();
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
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  if (sels.size() == 1) {
    VEngine::setPatternArgs((LedPos)sels[0], args);
  } else {
    // set the param on all patterns, which may require changing the pattern id
    for (uint32_t i = 0; i < sels.size(); ++i) {
      // only set the pattern on a single position
      VEngine::setSinglePat((LedPos)sels[i], VEngine::getPatternID((LedPos)pos));
      VEngine::setPatternArgs((LedPos)sels[i], args);
    }
    refreshFingerList(false);
  }
  // update the demo
  demoCurMode();
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

void VortexEditor::refreshModeList(bool recursive)
{
  m_modeListBox.clearItems();
  int curSel = VEngine::curMode();
  VEngine::setCurMode(0);
  for (uint32_t i = 0; i < VEngine::numModes(); ++i) {
    // just use the pattern name from the first pattern
    string modeName = "Mode " + to_string(i) + " (" + VEngine::getModeName() + ")";
    m_modeListBox.addItem(modeName);
    // go to next mode
    VEngine::nextMode();
  }
  // restore the selection
  m_modeListBox.setSelection(curSel);
  VEngine::setCurMode(curSel);
  if (recursive) {
    refreshFingerList(recursive);
  }
}

void VortexEditor::refreshFingerList(bool recursive)
{
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  m_fingersMultiListBox.clearItems();
  for (LedPos pos = LED_FIRST; pos < LED_COUNT; ++pos) {
    // if a finger is empty don't add it
    if (VEngine::getPatternID(pos) == PATTERN_NONE) {
      continue;
    }
    string fingerName = VEngine::ledToString(pos) + " (" + VEngine::getPatternName(pos) + ")";
    m_fingersMultiListBox.addItem(fingerName);
  }
  // restore the selection
  m_fingersMultiListBox.setSelections(sels);
  if (recursive) {
    refreshPatternSelect(recursive);
    refreshColorSelect(recursive);
    refreshParams(recursive);
    refreshApplyAll(recursive);
  }
}

void VortexEditor::refreshPatternSelect(bool recursive)
{
  if (!m_fingersMultiListBox.numItems() || !m_fingersMultiListBox.numSelections()) {
    m_patternSelectComboBox.setSelection(-1);
    m_patternSelectComboBox.setEnabled(false);
    return;
  }
  m_patternSelectComboBox.setEnabled(true);
  int sel = m_fingersMultiListBox.getSelection();
  if (sel < 0) {
    m_patternSelectComboBox.setSelection(PATTERN_NONE);
    return;
  }
  m_patternSelectComboBox.clearItems();
  bool allow_multi = (sel == 0);
  // get the pattern
  for (PatternID id = PATTERN_FIRST; id < PATTERN_COUNT; ++id) {
    bool isMulti = isMultiLedPatternID(id);
    if (!allow_multi && isMulti) {
      continue;
    }
    string patternName = VEngine::patternToString(id);
    if (isMulti) {
      patternName += " *";
    }
    m_patternSelectComboBox.addItem(patternName);
    if (id == VEngine::getPatternID((LedPos)sel)) {
      m_patternSelectComboBox.setSelection(id);
    }
  }
}

void VortexEditor::refreshColorSelect(bool recursive)
{
  if (!m_fingersMultiListBox.numItems() || !m_fingersMultiListBox.numSelections()) {
    for (uint32_t i = 0; i < 8; ++i) {
      m_colorSelects[i].clear();
      m_colorSelects[i].setActive(false);
    }
    return;
  }
  int pos = m_fingersMultiListBox.getSelection();
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

void VortexEditor::refreshParams(bool recursive)
{
  int sel = m_patternSelectComboBox.getSelection();
  if (sel < 0) {
    sel = 0;
  }
  int pos = m_fingersMultiListBox.getSelection();
  if (pos < 0) {
    for (uint32_t i = 0; i < 8; ++i) {
      m_paramTextBoxes[i].clearText();
      m_paramTextBoxes[i].setEnabled(false);
      m_paramTextBoxes[i].setVisible(false);
    }
    return;
  }
  vector<int> sels;
  m_fingersMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // disable all edit boxes but don't change their text, sorry.
    for (uint32_t i = 0; i < 8; ++i) {
      m_paramTextBoxes[i].setEnabled(false);
      m_paramTextBoxes[i].setVisible(false);
    }
    return;
  }
  vector<string> tips = VEngine::getCustomParams((PatternID)sel);
  if (sels.size() > 1) {
    bool all_same = true;
    PatternID base = VEngine::getPatternID((LedPos)sels[0]);
    for (uint32_t i = 1; i < sels.size(); ++i) {
      if (VEngine::getPatternID((LedPos)sels[i]) != base) {
        all_same = false;
      }
    }
    if (!all_same) {
      // disable all edit boxes but don't change their text, sorry.
      for (uint32_t i = 0; i < 8; ++i) {
        m_paramTextBoxes[i].setEnabled(false);
        m_paramTextBoxes[i].setVisible(false);
      }
      refreshApplyAll();
      return;
    }
  }
  PatternArgs args;
  VEngine::getPatternArgs((LedPos)pos, args);
  uint8_t *pArgs = (uint8_t *)&args.arg1;
  // get the number of params for the current pattern selection
  uint32_t numParams = VEngine::numCustomParams((PatternID)sel);
  // iterate all active params and activate
  for (uint32_t i = 0; i < numParams; ++i) {
    m_paramTextBoxes[i].setEnabled(false);
    m_paramTextBoxes[i].setText(to_string(pArgs[i]).c_str());
    m_paramTextBoxes[i].setEnabled(true);
    m_paramTextBoxes[i].setVisible(true);
    m_paramTextBoxes[i].setTooltip(tips[i]);
  }
  // iterate all extra slots and set to inactive
  for (uint32_t i = numParams; i < 8; ++i) {
    m_paramTextBoxes[i].clearText();
    m_paramTextBoxes[i].setEnabled(false);
    m_paramTextBoxes[i].setVisible(false);
  }
  // also refresh the apply to all button, why not
  refreshApplyAll();
}

void VortexEditor::refreshApplyAll(bool recursive)
{
  // also refresh the apply to all button, why not
  if (m_fingersMultiListBox.numSelections() == 1 &&
    !isMultiLedPatternID(VEngine::getPatternID())) {
    m_applyToAllButton.setEnabled(true);
  } else {
    m_applyToAllButton.setEnabled(false);
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
  int32_t actual = serial->ReadData((void *)outStream.data(), amt);
  if (actual != amt) {
    printf("FUUUCUASDKLFJHASDF\n");
    return false;
  }
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

bool VortexEditor::expectData(uint32_t port, const char *data)
{
  ByteStream stream;
  readInLoop(port, stream);
  if (stream.size() < strlen(data)) {
    return false;
  }
  if (strcmp((char *)stream.data(), data) != 0) {
    return false;
  }
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
  // first build a buffer with the size at the start
  uint8_t *buf = new uint8_t[size + sizeof(size)];
  if (!buf) {
    return;
  }
  memcpy(buf, &size, sizeof(size));
  memcpy(buf + sizeof(size), data.rawData(), size);
  // send the whole buffer in one go
  // NOTE: when I sent this in two sends it would actually cause the arduino
  // to only receive the size and not the buffer. It worked fine in the test
  // framework but not for arduino serial. So warning, always send in one chunk.
  // Even when I flushed the file buffers it didn't fix it.
  writePortRaw(portIndex, buf, size + sizeof(size));
  delete[] buf;
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
  if (!m_portList[sel].second.IsConnected()) {
    return false;
  }
  auto port = m_portActive.find(sel);
  if (port == m_portActive.end()) {
    return false;
  }
  return port->second;
}
