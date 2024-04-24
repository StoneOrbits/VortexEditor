#include "VortexEditor.h"

// VortexEngine includes
#include "Serial/ByteStream.h"
#include "Patterns/Patterns.h"
#include "Patterns/Pattern.h"
#include "Colors/Colorset.h"
#include "Random/Random.h"
#include "Modes/Mode.h"
#include "VortexLib.h"

// Editor includes
#include "ArduinoSerial.h"
#include "EditorConfig.h"
#include "GUI/VWindow.h"
#include "VortexPort.h"
#include "resource.h"

// stl includes
#include <algorithm>
#include <memory>
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
#define MOVE_MODE_UP_ID     50027
#define MOVE_MODE_DOWN_ID   50028

// user defined windows messages
#define WM_REFRESH_UI       WM_USER + 0 // refresh the UI
#define WM_TEST_CONNECT     WM_USER + 1 // new test framework connection
#define WM_TEST_DISCONNECT  WM_USER + 2 // test framework disconnect

// the prefix of colorsets copied to clipboard
#define COLORSET_CLIPBOARD_MARKER "COLORSET:"
// the prefix of leds copied to clipboard
#define LED_CLIPBOARD_MARKER "LED:"

// savefile extensions
#define VORTEX_SAVE_EXTENSION ".vortex"
#define VORTEX_MODE_EXTENSION ".vtxmode"

using namespace std;

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
  m_vortex(),
  m_engine(m_vortex.engine()),
  m_hInstance(NULL),
  m_hIcon(NULL),
  m_consoleHandle(nullptr),
  m_portList(),
  m_accelTable(),
  m_lastClickedColor(0),
  m_window(),
  m_portSelection(),
  m_pushButton(),
  m_pullButton(),
  m_modeListBox(),
  m_addModeButton(),
  m_delModeButton(),
  m_copyModeButton(),
  m_moveModeUpButton(),
  m_moveModeDownButton(),
  m_ledsMultiListBox(),
  m_patternSelectComboBox(),
  m_colorSelects(),
  m_paramTextBoxes()
{
}

VortexEditor::~VortexEditor()
{
  if (m_hIcon) {
    DestroyIcon(m_hIcon);
  }
}

class VortexEditorCallbacks : public VortexCallbacks
{
public:
  VortexEditorCallbacks(Vortex &vortex) : VortexCallbacks(vortex) {}
  virtual ~VortexEditorCallbacks() {}
  // called when engine reads digital pins, use this to feed button presses to the engine
  virtual long checkPinHook(uint32_t pin) { return 1; }
  // called when engine writes to ir, use this to read data from the vortex engine
  // the data received will be in timings of milliseconds
  // NOTE: to send data to IR use m_vortex.IRDeliver at any time
  virtual void infraredWrite(bool mark, uint32_t amount) { }
  // called when engine checks for Serial, use this to indicate serial is connected
  virtual bool serialCheck() { return false; }
  // called when engine begins serial, use this to do any initialization of the connection
  virtual void serialBegin(uint32_t baud) { }
  // called when engine checks for data on serial, use this to tell the engine data is ready
  virtual int32_t serialAvail() { return 0; }
  // called when engine reads from serial, use this to deliver data to the vortex engine
  // TODO: buffer this in libengine and add a deliver api like IR
  virtual size_t serialRead(char *buf, size_t amt) { return 0; }
  // called when engine writes to serial, use this to read data from the vortex engine
  virtual uint32_t serialWrite(const uint8_t *buf, size_t amt) { return 0; }
  // called when the LED strip is initialized
  virtual void ledsInit(void *cl, int count) { }
  // called when the brightness is changed
  virtual void ledsBrightness(int brightness) { }
  // called when the leds are shown
  virtual void ledsShow() { }
};

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
  DeleteFile("VortexEditor.dat");
#endif

  // initialize the system that wraps the vortex engine
  m_vortex.initEx<VortexEditorCallbacks>();

  // idk why not
  m_vortex.setLedCount(1);

  // initialize the window accordingly
  m_window.init(hInst, EDITOR_TITLE, BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT, g_pEditor, "VortexEditor");

  m_portSelection.init(hInst, m_window, "Select Port", BACK_COL, 72, 100, 16, 15, SELECT_PORT_ID, selectPortCallback);

  m_pullButton.init(hInst, m_window, "Pull", BACK_COL, 78, 24, 100, 15, ID_FILE_PULL, pullCallback);
  m_pushButton.init(hInst, m_window, "Push", BACK_COL, 78, 24, 188, 15, ID_FILE_PUSH, pushCallback);

  // status bar
  m_statusBar.init(hInst, m_window, "", BACK_COL, 462, 24, 278, 15, 0, nullptr);
  m_statusBar.setForeEnabled(true);
  m_statusBar.setBackEnabled(true);
  m_statusBar.setStatus(RGB(255, 0, 0), "Disconnected");

  m_modeListBox.init(hInst, m_window, "Mode List", BACK_COL, 250, 270, 16, 54, SELECT_MODE_ID, selectModeCallback);

  uint32_t buttonWidth = 46;
  uint32_t buttonHeight = 24;
  m_addModeButton.init(hInst, m_window, "Add", BACK_COL, buttonWidth, buttonHeight, 16, 320, ADD_MODE_ID, addModeCallback);
  m_delModeButton.init(hInst, m_window, "Del", BACK_COL, buttonWidth, buttonHeight, 60, 320, DEL_MODE_ID, delModeCallback);
  m_copyModeButton.init(hInst, m_window, "Copy", BACK_COL, buttonWidth, buttonHeight, 110, 320, COPY_MODE_ID, copyModeCallback);
  m_moveModeUpButton.init(hInst, m_window, "Up", BACK_COL, buttonWidth, buttonHeight, 110, 320, MOVE_MODE_UP_ID, moveModeUpCallback);
  m_moveModeDownButton.init(hInst, m_window, "Down", BACK_COL, buttonWidth, buttonHeight, 110, 320, MOVE_MODE_DOWN_ID, moveModeDownCallback);

  vector<VWindow *> modeButtonList = {
    &m_addModeButton, &m_delModeButton, &m_copyModeButton, &m_moveModeUpButton, &m_moveModeDownButton
  };

  // starting position for buttons
  uint32_t startPos = 16;
  // how much space between each button
  uint32_t buttonSep = 5;
  // position all the buttons along the top
  for (uint32_t i = 0; i < modeButtonList.size(); ++i) {
    SetWindowPos(modeButtonList[i]->hwnd(), NULL, startPos + (i * (buttonWidth + buttonSep)), 320, 0, 0, SWP_NOSIZE);
  }

  m_ledsMultiListBox.init(hInst, m_window, "Fingers", BACK_COL, 230, 305, 278, 54, SELECT_FINGER_ID, selectFingerCallback);
  m_patternSelectComboBox.init(hInst, m_window, "Select Pattern", BACK_COL, 165, 300, 520, 54, SELECT_PATTERN_ID, selectPatternCallback);

  for (uint32_t i = 0; i < 8; ++i) {
    m_colorSelects[i].init(hInst, m_window, "Color Select", BACK_COL, 36, 30, 520, 83 + (33 * i), SELECT_COLOR_ID + i, selectColorCallback);
  }

  for (uint32_t i = 0; i < 8; ++i) {
    m_paramTextBoxes[i].init(hInst, m_window, "", BACK_COL, buttonWidth, 24, 693, 54 + (32 * i), PARAM_EDIT_ID + i, paramEditCallback);
  }

  m_storageProgress.init(hInst, m_window, "", BACK_COL, EDITOR_WIDTH, 2, 0, 0, 0, nullptr);
  m_storageProgress.setDrawHLine(false);
  m_storageProgress.setDrawCircle(false);
  m_storageProgress.setTooltip("Storage Space");

  // install callback for all menu IDs, these could be separate, idk
  m_window.addCallback(ID_COLORSET_RANDOM_COMPLIMENTARY, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_MONOCHROMATIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_TRIADIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_SQUARE, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_PENTADIC, handleMenusCallback);
  m_window.addCallback(ID_COLORSET_RANDOM_RAINBOW, handleMenusCallback);
  m_window.addCallback(ID_PATTERN_RANDOM_SINGLE_LED_PATTERN, handleMenusCallback);
  m_window.addCallback(ID_PATTERN_RANDOM_MULTI_LED_PATTERN, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_LED, handleMenusCallback);
  m_window.addCallback(ID_EDIT_PASTE_LED, handleMenusCallback);
  m_window.addCallback(ID_EDIT_CLEAR_COLORSET, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_COLOR_SET_TO_ALL, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_PATTERN_TO_ALL, handleMenusCallback);
  m_window.addCallback(ID_HELP_ABOUT, handleMenusCallback);
  m_window.addCallback(ID_HELP_TUTORIAL, handleMenusCallback);
  m_window.addCallback(ID_HELP_WIKI, handleMenusCallback);
  m_window.addCallback(ID_EDIT_COPY_COLORSET, handleMenusCallback);
  m_window.addCallback(ID_EDIT_PASTE_COLORSET, handleMenusCallback);
  m_window.addCallback(ID_EDIT_CLEAR_PATTERN, handleMenusCallback);
  m_window.addCallback(ID_OPTIONS_TRANSMIT_DUO, handleMenusCallback);
  m_window.addCallback(ID_OPTIONS_TRANSMIT_INFRARED, handleMenusCallback);
  m_window.addCallback(ID_OPTIONS_RECEIVE_FROM_DUO, handleMenusCallback);
  m_window.addCallback(ID_EDIT_UNDO, handleMenusCallback);
  m_window.addCallback(ID_EDIT_REDO, handleMenusCallback);
  m_window.addCallback(ID_FILE_PULL, handleMenusCallback);
  m_window.addCallback(ID_FILE_PUSH, handleMenusCallback);
  m_window.addCallback(ID_FILE_LOAD, handleMenusCallback);
  m_window.addCallback(ID_FILE_SAVE, handleMenusCallback);
  m_window.addCallback(ID_FILE_IMPORT, handleMenusCallback);
  m_window.addCallback(ID_FILE_EXPORT, handleMenusCallback);
  m_window.addCallback(ID_TOOLS_COLOR_PICKER, handleMenusCallback);
  m_window.addCallback(ID_TOOLS_MODE_RANDOMIZER, handleMenusCallback);
  m_window.addCallback(ID_TOOLS_COMMUNITY_BROWSER, handleMenusCallback);

  // add user callback for refreshes
  m_window.installUserCallback(WM_REFRESH_UI, refreshWindowCallback);
  m_window.installUserCallback(WM_TEST_CONNECT, connectTestFrameworkCallback);
  m_window.installUserCallback(WM_TEST_DISCONNECT, disconnectTestFrameworkCallback);

  // current window pos for child window init
  RECT pos;
  GetWindowRect(m_window.hwnd(), &pos);

  // initialize the color picker window
  m_colorPicker.init(hInst);
  SetWindowPos(m_colorPicker.hwnd(), 0, pos.left - 422, pos.top-200, 0, 0, SWP_NOSIZE);

  // initialize the mode randomizer window
  m_modeRandomizer.init(hInst);
  SetWindowPos(m_modeRandomizer.hwnd(), 0, pos.left + 50, pos.top-300, 0, 0, SWP_NOSIZE);
 
  // initialize the community browser window
  m_communityBrowser.init(hInst);
  SetWindowPos(m_communityBrowser.hwnd(), 0, pos.right + 2, pos.top - 200, 0, 0, SWP_NOSIZE);

  // initialize the tutorial
  m_tutorial.init(hInst);
  SetWindowPos(m_tutorial.hwnd(), 0, pos.left + 180, pos.top + 50, 0, 0, SWP_NOSIZE);

  // show subwindows
  //m_colorPicker.show();
  //m_modeRandomizer.show();
  //m_communityBrowser.show();

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_window.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  // create an accelerator table for dispatching hotkeys as WM_COMMANDS
  // for specific menu IDs
  ACCEL accelerators[] = {
    // ctrl + z   undo
    { FCONTROL | FVIRTKEY, 'Z', ID_EDIT_UNDO },
    // ctrl + y   redo
    { FCONTROL | FVIRTKEY, 'Y', ID_EDIT_REDO },
    // shh...
    { FCONTROL | FVIRTKEY, 'R', ID_EDIT_REDO },
    // ctrl + c   copy led
    { FCONTROL | FVIRTKEY, 'C', ID_EDIT_COPY_LED },
    // ctrl + v   paste led
    { FCONTROL | FVIRTKEY, 'V', ID_EDIT_PASTE_LED },
    // ctrl + shift + D   clear colorset
    { FCONTROL | FSHIFT | FVIRTKEY, 'D', ID_EDIT_CLEAR_COLORSET },
    // ctrl + shift + C   copy colorset
    { FCONTROL | FSHIFT | FVIRTKEY, 'C', ID_EDIT_COPY_COLORSET },
    // ctrl + shift + V   paste colorset
    { FCONTROL | FSHIFT | FVIRTKEY, 'V', ID_EDIT_PASTE_COLORSET },
    // ctrl + e  pull
    { FCONTROL | FVIRTKEY, 'E', ID_FILE_PULL },
    // ctrl + t  push
    { FCONTROL | FVIRTKEY, 'T', ID_FILE_PUSH },
    // ctrl + s  save
    { FCONTROL | FVIRTKEY, 'S', ID_FILE_SAVE },
    // ctrl + o  open
    { FCONTROL | FVIRTKEY, 'O', ID_FILE_LOAD },
    // ctrl + shift + S  save
    { FCONTROL | FSHIFT | FVIRTKEY, 'S', ID_FILE_EXPORT },
    // ctrl + shift + O  open
    { FCONTROL | FSHIFT | FVIRTKEY, 'O', ID_FILE_IMPORT },
    // ctrl + d
    { FCONTROL | FVIRTKEY, 'D', ID_EDIT_CLEAR_PATTERN },
    // ctrl + u
    { FCONTROL | FVIRTKEY, 'U', ID_OPTIONS_TRANSMIT_DUO },
    // ctrl + i
    { FCONTROL | FVIRTKEY, 'I', ID_OPTIONS_TRANSMIT_INFRARED },
    // ctrl + k
    { FCONTROL | FVIRTKEY, 'K', ID_OPTIONS_RECEIVE_FROM_DUO },
    // ctrl + 1
    { FCONTROL | FVIRTKEY, '1', ID_COLORSET_RANDOM_MONOCHROMATIC },
    // ctrl + 2
    { FCONTROL | FVIRTKEY, '2', ID_COLORSET_RANDOM_COMPLIMENTARY },
    // ctrl + 3
    { FCONTROL | FVIRTKEY, '3', ID_COLORSET_RANDOM_TRIADIC },
    // ctrl + 4
    { FCONTROL | FVIRTKEY, '4', ID_COLORSET_RANDOM_SQUARE },
    // ctrl + 5
    { FCONTROL | FVIRTKEY, '5', ID_COLORSET_RANDOM_PENTADIC },
    // ctrl + 6
    { FCONTROL | FVIRTKEY, '6', ID_COLORSET_RANDOM_RAINBOW },
  };
  m_accelTable = CreateAcceleratorTable(accelerators, sizeof(accelerators) / sizeof(accelerators[0]));
  if (!m_accelTable) {
    // error!
  }

  // install the device callback
  m_window.installDeviceCallback(deviceChangeCallback);

  // check for connected devices
  scanPorts();

  // trigger a ui refresh
  refreshModeList();

  return true;
}

void VortexEditor::run()
{
  // main message loop
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (GetForegroundWindow() == m_window.hwnd()) {
      if (TranslateAccelerator(m_window.hwnd(), m_accelTable, &msg)) {
        continue;
      }
    }
    // pass message to main window otherwise process it
    if (!m_window.process(msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

void VortexEditor::triggerRefresh()
{
  PostMessage(m_window.hwnd(), WM_USER, 0, 0);
}

void VortexEditor::printlog(const char *file, const char *func, int line, const char *msg, ...)
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
  va_list list;
  va_start(list, msg);
  vfprintf(g_pEditor->m_consoleHandle, strMsg.c_str(), list);
  va_end(list);
  //vfprintf(g_pEditor->m_logHandle, strMsg.c_str(), list);
}

void VortexEditor::handleMenus(uintptr_t hMenu)
{
  switch (hMenu) {
  case ID_HELP_ABOUT:
    MessageBox(m_window.hwnd(), "Vortex Editor 1.0\nMade by Daniel Fraser and Shane Aronson", "About", 0);
    break;
  case ID_HELP_WIKI:
    ShellExecute(NULL, "open", "https://stoneorbits.github.io/VortexEngine/editor.html", NULL, NULL, SW_SHOWNORMAL);
    return;
  case ID_HELP_TUTORIAL:
    beginTutorial();
    return;
  case ID_EDIT_COPY_COLORSET:
    copyColorset();
    return;
  case ID_EDIT_PASTE_COLORSET:
    pasteColorset();
    return;
  case ID_EDIT_COPY_LED:
    copyLED();
    return;
  case ID_EDIT_PASTE_LED:
    pasteLED();
    return;
  case ID_EDIT_CLEAR_PATTERN:
    clearLED();
    return;
  case ID_EDIT_UNDO:
    m_vortex.undo();
    refreshModeList();
    return;
  case ID_EDIT_REDO:
    m_vortex.redo();
    refreshModeList();
    return;
  case ID_FILE_PULL:
    pull(nullptr);
    return;
  case ID_FILE_PUSH:
    push(nullptr);
    return;
  case ID_FILE_LOAD:
    load(nullptr);
    return;
  case ID_FILE_SAVE:
    save(nullptr);
    return;
  case ID_FILE_IMPORT:
    importMode(nullptr);
    return;
  case ID_FILE_EXPORT:
    exportMode(nullptr);
    return;
  case ID_OPTIONS_TRANSMIT_DUO:
    transmitVL(nullptr);
    return;
  case ID_OPTIONS_TRANSMIT_INFRARED:
    transmitIR(nullptr);
    return;
  case ID_OPTIONS_RECEIVE_FROM_DUO:
    receiveVL(nullptr);
    return;
  case ID_TOOLS_COLOR_PICKER:
    m_colorPicker.show();
    return;
  case ID_TOOLS_MODE_RANDOMIZER:
    m_modeRandomizer.show();
    return;
  case ID_TOOLS_COMMUNITY_BROWSER:
    m_communityBrowser.show();
    return;
  default:
    break;
  }
  uintptr_t menu = (uintptr_t)hMenu;
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  // TODO seed me properly?
  Random ctx((uint32_t)time(NULL));
  Colorset newSet;
  switch (menu) {
  case ID_COLORSET_RANDOM_COMPLIMENTARY:
    newSet.randomizeComplimentary(ctx);
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_MONOCHROMATIC:
    newSet.randomizeColors(ctx, 0, Colorset::ColorMode::MONOCHROMATIC);
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_TRIADIC:
    newSet.randomizeTriadic(ctx);
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_SQUARE:
    newSet.randomizeSquare(ctx);
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_PENTADIC:
    newSet.randomizePentadic(ctx);
    applyColorset(newSet, sels);
    break;
  case ID_COLORSET_RANDOM_RAINBOW:
    newSet.randomizeRainbow(ctx);
    applyColorset(newSet, sels);
    break;
  case ID_PATTERN_RANDOM_SINGLE_LED_PATTERN:
    // when applying a single led pattern we must use the 'setPattern' api
    // to properly convert the mode to all-same-single if it's a multi to
    // begin with, otherwise we can just apply the single to whichever we select
    if (isMultiLedPatternID(m_vortex.getPatternID(LED_ANY))) {
      m_vortex.setPattern((PatternID)ctx.next16(PATTERN_FIRST, PATTERN_SINGLE_LAST));
    } else {
      for (uint32_t i = 0; i < sels.size(); ++i) {
        m_vortex.setPatternAt((LedPos)sels[i], (PatternID)ctx.next16(PATTERN_FIRST, PATTERN_SINGLE_LAST));
      }
    }
    refreshModeList();
    demoCurMode();
    break;
  case ID_PATTERN_RANDOM_MULTI_LED_PATTERN:
    m_vortex.setPattern((PatternID)ctx.next16(PATTERN_MULTI_FIRST, PATTERN_MULTI_LAST));
    refreshModeList();
    demoCurMode();
    break;
  case ID_EDIT_CLEAR_COLORSET:
    newSet.clear();
    applyColorset(newSet, sels);
    break;
#if 0
  // this is kinda pointless if we have ctrl+c/ctrl+v
  case ID_EDIT_COPY_COLOR_SET_TO_ALL:
    if (sels.size() != 1) {
      break;
    }
    m_vortex.getColorset((LedPos)sels[0], newSet);
    applyColorsetToAll(newSet);
    break;
  case ID_EDIT_COPY_PATTERN_TO_ALL:
    if (sels.size() != 1) {
      break;
    }
    applyPatternToAll(m_vortex.getPatternID((LedPos)sels[0]));
    break;
#endif
  }
}

void VortexEditor::deviceChange(DEV_BROADCAST_HDR *dbh, bool added)
{
  if (dbh->dbch_devicetype != DBT_DEVTYP_PORT) {
    return;
  }
  DEV_BROADCAST_PORT *dbp = (DEV_BROADCAST_PORT *)dbh;
  if (!dbp->dbcp_name) {
    return;
  }
  string portName = dbp->dbcp_name;
  uint32_t portNum = strtoul(portName.c_str() + 3, NULL, 10);
  debug("%s: %u\n", added ? "Connected" : "Disconnected", portNum);
  if (added) {
    connectPort(portNum);
  } else {
    disconnectPort(portNum);
  }
}

void VortexEditor::updateSelectedColors(uint32_t rawCol)
{
  // get list of all selected colors
  for (uint32_t i = 0; i < 8; ++i) {
    if (m_colorSelects[i].isSelected()) {
      updateSelectedColor(m_colorSelects + i, rawCol, false);
    }
  }
  // refresh and update the demo
  refreshModeList();
  demoCurMode();
}

void VortexEditor::updateSelectedColor(VColorSelect *colSelect, uint32_t rawCol, bool demo)
{
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  uint32_t colorIndex = (uint32_t)((uintptr_t)colSelect->menu() - SELECT_COLOR_ID);
  colSelect->setColor(rawCol);
  Colorset newSet;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getColorset((LedPos)pos, newSet);
  // if the color select was made inactive
  if (!colSelect->isActive()) {
    debug("Disabled color slot");
    newSet.removeColor(colorIndex);
  } else {
    debug("Updating color slot");
    newSet.set(colorIndex, colSelect->getColor()); // getRawColor?
  }
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setColorset(LED_MULTI, newSet);
  } else {
    for (uint32_t i = 0; i < sels.size(); ++i) {
      m_vortex.setColorset((LedPos)sels[i], newSet);
    }
  }
  if (demo) {
    // refresh and update the demo
    refreshModeList();
    demoCurMode();
  }
}

void VortexEditor::applyColorset(const Colorset &set, const vector<int> &selections)
{
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setColorset(LED_MULTI, set);
  } else {
    for (uint32_t i = 0; i < selections.size(); ++i) {
      m_vortex.setColorset((LedPos)selections[i], set);
    }
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyPattern(PatternID id, const vector<int> &selections)
{
  for (uint32_t i = 0; i < selections.size(); ++i) {
    m_vortex.setPatternAt((LedPos)selections[i], id);
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyColorsetToAll(const Colorset &set)
{
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setColorset(LED_MULTI, set);
  } else {
    for (uint32_t i = 0; i < m_vortex.numLedsInMode(); ++i) {
      m_vortex.setColorset((LedPos)i, set);
    }
  }
  refreshColorSelect();
  // update the demo
  demoCurMode();
}

void VortexEditor::applyPatternToAll(PatternID id)
{
  for (LedPos i = LED_FIRST; i < m_vortex.numLedsInMode(); ++i) {
    m_vortex.setPatternAt(i, id);
  }
  refreshLedList();
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
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  string colorset;
  getClipboard(colorset);
  // check for the colorset marker
  if (strncmp(colorset.c_str(), COLORSET_CLIPBOARD_MARKER, sizeof(COLORSET_CLIPBOARD_MARKER) - 1) != 0) {
    return;
  }
  vector<string> splits;
  splitString(colorset.c_str() + sizeof(COLORSET_CLIPBOARD_MARKER) - 1, splits, ',');
  Colorset newSet;
  for (auto field : splits) {
    if (field == "blank" || field[0] != '#') {
      newSet.addColor(0);
    } else {
      newSet.addColor(strtoul(field.c_str() + 1, NULL, 16));
    }
  }
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setColorset(LED_MULTI, newSet);
  } else {
    for (uint32_t i = 0; i < sels.size(); ++i) {
      m_vortex.setColorset((LedPos)sels[i], newSet);
    }
  }
  refreshColorSelect();
  demoCurMode();
}

void VortexEditor::copyLED()
{
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  // TODO: led/colorset to/from json
  string led = LED_CLIPBOARD_MARKER;
  led += to_string(patternSelection()) + ";";
  PatternArgs args;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getPatternArgs((LedPos)pos, args);
  led += to_string(args.arg1) + ",";
  led += to_string(args.arg2) + ",";
  led += to_string(args.arg3) + ",";
  led += to_string(args.arg4) + ",";
  led += to_string(args.arg5) + ",";
  led += to_string(args.arg6);
  led += ";";
  for (uint32_t i = 0; i < 8; ++i) {
    if (!m_colorSelects[i].isActive()) {
      break;
    }
    string name = m_colorSelects[i].getColorName();
    if (i > 0) {
      led += ",";
    }
    led += name;
  }
  setClipboard(led);
}

void VortexEditor::splitString(const string &str, vector<string> &splits, char letter)
{
  string split;
  istringstream ss(str);
  while (getline(ss, split, letter)) {
    splits.push_back(split);
  }
}

void VortexEditor::pasteLED()
{
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  // TODO: this is so ugly
  string led;
  getClipboard(led);
  // check for the colorset marker
  if (strncmp(led.c_str(), LED_CLIPBOARD_MARKER, sizeof(LED_CLIPBOARD_MARKER) - 1) != 0) {
    return;
  }
  // split the string by semicolon
  vector<string> splits;
  splitString(led.c_str() + sizeof(LED_CLIPBOARD_MARKER) - 1, splits, ';');
  if (splits.size() < 3) {
    return;
  }
  // pattern id is first
  PatternID id = (PatternID)strtoul(splits[0].c_str(), NULL, 10);
  // pattern args are second
  PatternArgs args;
  vector<string> argSplit;
  splitString(splits[1].c_str(), argSplit, ',');
  if (argSplit.size() < 6) {
    return;
  }
  // convert args
  args.arg1 = (uint8_t)strtoul(argSplit[0].c_str(), NULL, 10);
  args.arg2 = (uint8_t)strtoul(argSplit[1].c_str(), NULL, 10);
  args.arg3 = (uint8_t)strtoul(argSplit[2].c_str(), NULL, 10);
  args.arg4 = (uint8_t)strtoul(argSplit[3].c_str(), NULL, 10);
  args.arg5 = (uint8_t)strtoul(argSplit[4].c_str(), NULL, 10);
  args.arg6 = (uint8_t)strtoul(argSplit[5].c_str(), NULL, 10);
  // convert colorset
  Colorset newSet;
  vector<string> colorSplit;
  splitString(splits[2].c_str(), colorSplit, ',');
  for (auto field : colorSplit) {
    if (field == "blank" || field[0] != '#') {
      newSet.addColor(0);
    } else {
      newSet.addColor(strtoul(field.c_str() + 1, NULL, 16));
    }
  }
  // if applying multi-led, or changing multi-to single
  if (isMultiLedPatternID(id) || isMultiLedPatternID(m_vortex.getPatternID(LED_ANY))) {
    // then just set-all
    m_vortex.setPattern(id, &args, &newSet);
  } else {
    // otherwise set single
    for (uint32_t i = 0; i < sels.size(); ++i) {
      m_vortex.setPatternAt((LedPos)sels[i], id, &args, &newSet);
    }
  }
  refreshModeList();
  demoCurMode();
}

void VortexEditor::clearLED()
{
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  // clear pattern at each position
  for (uint32_t i = 0; i < sels.size(); ++i) {
    m_vortex.setPatternAt((LedPos)sels[i], PATTERN_NONE);
  }
  refreshModeList();
  demoCurMode();
}

void VortexEditor::getClipboard(string &clipData)
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

void VortexEditor::setClipboard(const string &clipData)
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

void VortexEditor::scanPorts()
{
  for (uint32_t i = 0; i < 255; ++i) {
    if (isPortConnected(i)) {
      continue;
    }
    connectPort(i);
  }
}

void VortexEditor::connectPort(uint32_t portNum)
{
  if (isPortConnected(portNum)) {
    return;
  }
  string portStr = "\\\\.\\COM" + to_string(portNum);
  if (portNum == 0) {
    portStr = "\\\\.\\pipe\\vortextestframework";
  }
  unique_ptr<VortexPort> port = make_unique<VortexPort>(portStr);
  if (port->isConnected()) {
    port->listen();
    m_portList.push_back(make_pair(portNum, move(port)));
  }
  refreshPortList();
}

void VortexEditor::disconnectPort(uint32_t portNum)
{
  if (!m_portList.size()) {
    return;
  }
  for (uint32_t i = 0; i < m_portList.size(); ++i) {
    if (m_portList[i].first != portNum) {
      continue;
    }
    // are we deleting the one we have selected?
    int sel = getPortListIndex();
    if (sel != -1 && (uint32_t)sel >= i) {
      m_portSelection.setSelection(sel - 1);
    }
    m_portList.erase(m_portList.begin() + i);
    refreshPortList();
    break;
  }
}

void VortexEditor::selectPort(VWindow *window)
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  // try to begin operations on port
  port->tryBegin();
  refreshPortList();
  // refresh the status
  refreshStatus();
}

void VortexEditor::push(VWindow *window)
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  // send the push modes command
  port->writeData(EDITOR_VERB_PUSH_MODES);
  // read data again
  port->expectData(EDITOR_VERB_READY);
  // now unserialize the stream of data that was read
  ByteStream modes;
  m_vortex.getModes(modes);
  // send the modes
  port->writeData(modes);
  // wait for the done response
  port->expectData(EDITOR_VERB_PUSH_MODES_ACK);
}

void VortexEditor::pull(VWindow *window)
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  ByteStream stream;
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_PULL_MODES);
  stream.clear();
  if (!port->readByteStream(stream) || !stream.size()) {
    debug("Couldn't read anything");
    return;
  }
  m_vortex.matchLedCount(stream, false);
  m_vortex.setModes(stream);
  // now send the done message
  port->writeData(EDITOR_VERB_PULL_MODES_DONE);
  // wait for the ack from the gloves
  port->expectData(EDITOR_VERB_PULL_MODES_ACK);
  // unserialized all our modes
  debug("Unserialized %u modes", m_vortex.numModes());
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
  ofn.lpstrFilter = "Vortex Save\0*" VORTEX_SAVE_EXTENSION "\0";
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
  m_vortex.setModes(stream);
  debug("Loaded from [%s]", szFile);
  refreshModeList();
  demoCurMode();
}

void VortexEditor::save(VWindow *window)
{
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  char szFile[MAX_PATH] = "ModesBackup" VORTEX_SAVE_EXTENSION;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Save\0*" VORTEX_SAVE_EXTENSION "\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST;
  if (!GetSaveFileName(&ofn)) {
    return;
  }
  string filename = szFile;
  if (filename.length() <= strlen(VORTEX_SAVE_EXTENSION)) {
    return;
  }
  if (filename.substr(filename.length() - strlen(VORTEX_SAVE_EXTENSION)) != VORTEX_SAVE_EXTENSION) {
    filename.append(VORTEX_SAVE_EXTENSION);
  }
  HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  ByteStream stream;
  m_vortex.getModes(stream);
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  debug("Saved to [%s]", filename.c_str());
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
  ofn.lpstrFilter = "Vortex Mode\0*"  VORTEX_MODE_EXTENSION "\0";
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
  if (!m_vortex.addNewMode(stream)) {
    // error
  }
  debug("Loaded from [%s]", szFile);
  refreshModeList();
  demoCurMode();
}

void VortexEditor::exportMode(VWindow *window)
{
  if (!m_vortex.numModes()) {
    return;
  }
  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  string modeName = "Mode_" + to_string(m_vortex.curModeIndex()) + "_" + m_vortex.getModeName();
  replace(modeName.begin(), modeName.end(), ' ', '_');
  modeName += VORTEX_MODE_EXTENSION;
  char szFile[MAX_PATH] = {0};
  memcpy(szFile, modeName.c_str(), modeName.length());
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Vortex Mode\0*" VORTEX_MODE_EXTENSION "\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST;
  if (!GetSaveFileName(&ofn)) {
    return;
  }
  string filename = szFile;
  if (filename.length() <= strlen(VORTEX_MODE_EXTENSION)) {
    return;
  }
  if (filename.substr(filename.length() - strlen(VORTEX_MODE_EXTENSION)) != VORTEX_MODE_EXTENSION) {
    filename.append(VORTEX_MODE_EXTENSION);
  }
  HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!hFile) {
    // error
    return;
  }
  DWORD written = 0;
  ByteStream stream;
  m_vortex.getCurMode(stream);
  if (!WriteFile(hFile, stream.rawData(), stream.rawSize(), &written, NULL)) {
    // error
  }
  CloseHandle(hFile);
  debug("Saved to [%s]", filename.c_str());
}

void VortexEditor::transmitVL(VWindow *window)
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0 || !isConnected()) {
    return;
  }
  // now unserialize the stream of data that was read
  ByteStream curMode;
  if (!m_vortex.getCurMode(curMode) || curMode.size() <= 4) {
    // error!
    // TODO: abort
    return;
  }
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_TRANSMIT_VL);
  // read data again
  port->expectData(EDITOR_VERB_TRANSMIT_VL_ACK);
}

void VortexEditor::transmitIR(VWindow *window)
{
}

void VortexEditor::receiveVL(VWindow *window)
{
#if 0
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0 || !isConnected()) {
    return;
  }
  // now unserialize the stream of data that was read
  ByteStream curMode;
  if (!m_vortex.getCurMode(curMode) || curMode.size() <= 4) {
    // error!
    // TODO: abort
    return;
  }
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_LISTEN_VL);
  // listen for he mode
  ByteStream stream;
  stream.clear();
  if (!port->readByteStream(stream) || !stream.size()) {
    debug("Couldn't read anything");
    return;
  }
  m_vortex.setLedCount(2);
  m_vortex.addNewMode(stream);
  // read data again
  port->expectData(EDITOR_VERB_LISTEN_VL_ACK);
  // refresh
  refreshModeList();
  // select the new mode
  m_modeListBox.setSelection(m_vortex.numModes() - 1);
  m_ledsMultiListBox.setSelection(0);
  refreshLedList();
#endif
}

void VortexEditor::selectMode(VWindow *window)
{
  int sel = m_modeListBox.getSelection();
  if (sel == m_vortex.curModeIndex()) {
    // trigger demo again w/e
    demoCurMode();
    return;
  }
  if (sel < 0) {
    return;
  }
  if (!m_vortex.setCurMode(sel)) {
    // error!
    return;
  }
  // reselect first led
  m_ledsMultiListBox.clearSelections();
  m_ledsMultiListBox.setSelection(0);
  refreshLedList();
  demoCurMode();
}

void VortexEditor::demoCurMode()
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0 || !isConnected()) {
    return;
  }
  // now unserialize the stream of data that was read
  ByteStream curMode;
  if (!m_vortex.getCurMode(curMode) || curMode.size() <= 4) {
    // error!
    // TODO: abort
    return;
  }
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_DEMO_MODE);
  // read data again
  port->expectData(EDITOR_VERB_READY);
  // send, the, mode
  port->writeData(curMode);
  // wait for the done response
  port->expectData(EDITOR_VERB_DEMO_MODE_ACK);
  string modeName = "Mode_" + to_string(m_vortex.curModeIndex()) + "_" + m_vortex.getModeName();
  // Set status? maybe soon
  //m_statusBar.setStatus(RGB(0, 255, 255), ("Demoing " + modeName).c_str());
}

void VortexEditor::clearDemo()
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_CLEAR_DEMO);
  // read data again
  port->expectData(EDITOR_VERB_CLEAR_DEMO_ACK);
}

void VortexEditor::addMode(VWindow *window)
{
#if MAX_MODES != 0
  if (m_vortex.numModes() >= MAX_MODES) {
    return;
  }
#endif
  debug("Adding mode %u", m_vortex.numModes() + 1);
  m_vortex.addNewMode();
  m_modeListBox.setSelection(m_vortex.curModeIndex());
  refreshModeList();
  if (m_vortex.numModes() == 1) {
    m_ledsMultiListBox.setSelection(0);
    refreshModeList();
    demoCurMode();
  }
}

void VortexEditor::addMode(VWindow *window, const Mode *mode)
{
  if (!mode) {
    return;
  }
#if MAX_MODES != 0
  if (m_vortex.numModes() >= MAX_MODES) {
    return;
  }
#endif
  debug("Adding mode %u", m_vortex.numModes() + 1);
  m_vortex.addMode(mode);
  m_vortex.setCurMode(m_vortex.numModes() - 1);
  m_modeListBox.setSelection(m_vortex.curModeIndex());
  refreshModeList();
  demoCurMode();
}

void VortexEditor::delMode(VWindow *window)
{
  debug("Deleting mode %u", m_vortex.curModeIndex());
  uint32_t cur = m_vortex.curModeIndex();
  m_vortex.delCurMode();
  refreshModeList();
  if (!m_vortex.numModes()) {
    clearDemo();
  } else {
    demoCurMode();
  }
}

void VortexEditor::copyMode(VWindow *window)
{
  if (!m_vortex.numModes()) {
    return;
  }
  int sel = m_modeListBox.getSelection();
  if (sel < 0) {
    return;
  }
  debug("Copying mode %u", m_vortex.curModeIndex());
  ByteStream stream;
  m_vortex.getCurMode(stream);
  m_vortex.addNewMode(stream);
  refreshModeList();
}

void VortexEditor::moveModeUp(VWindow *window)
{
  m_vortex.shiftCurMode(-1);
  refreshModeList();
}

void VortexEditor::moveModeDown(VWindow *window)
{
  m_vortex.shiftCurMode(1);
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
  PatternID pat = patternSelection();
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    return;
  }
  // if we ONLY selected the first led
  if (sels.size() == 1 && sels[0] == 0) {
    // and if we are switching from a multi-led or to a multi-led
    if (m_vortex.isCurModeMulti() || isMultiLedPatternID(pat)) {
      // then set the pattern on the entire mode
      m_vortex.setPattern(pat);
    } else {
      // otherwise we are switching from single to single to just
      // apply the pattern change to this slot
      m_vortex.setPatternAt(LED_FIRST, pat);
    }
  } else {
    for (uint32_t i = 0; i < sels.size(); ++i) {
      // only set the pattern on a single position
      m_vortex.setPatternAt((LedPos)sels[i], pat);
    }
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::copyToAll(VWindow *window)
{
  PatternID pat = patternSelection();
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (sels.size() > 1) {
    return;
  }
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  if (isMultiLedPatternID(pat)) {
    return;
  }
  PatternArgs args;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getPatternArgs((LedPos)pos, args);
  Colorset set;
  m_vortex.getColorset((LedPos)pos, set);
  for (LedPos i = LED_FIRST; i < m_vortex.numLedsInMode(); ++i) {
    if (pos == i) {
      continue;
    }
    m_vortex.setPatternAt(i, pat, &args, &set);
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
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    return;
  }
  uint32_t paramIndex = (uint32_t)((uintptr_t)window->menu() - PARAM_EDIT_ID);
  PatternArgs args;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getPatternArgs((LedPos)pos, args);
  // get the number of params for the current pattern selection
  uint32_t numParams = m_vortex.numCustomParams(patternSelection());
  // store the target param
  args.args[paramIndex] = m_paramTextBoxes[paramIndex].getValue();
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
    // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setPatternArgs(LED_MULTI, args);
  } else {
    if (sels.size() == 1) {
      m_vortex.setPatternArgs((LedPos)sels[0], args);
    } else {
      // set the param on all patterns, which may require changing the pattern id
      for (uint32_t i = 0; i < sels.size(); ++i) {
        m_vortex.setPatternAt((LedPos)sels[i], m_vortex.getPatternID((LedPos)pos), &args);
      }
      refreshLedList(false);
    }
  }
  // update the demo
  demoCurMode();
}

void VortexEditor::selectColor(VColorSelect *colSelect, VColorSelect::SelectEvent sevent)
{
  if (!colSelect) {
    return;
  }
  uint32_t colorIndex = (uint32_t)((uintptr_t)colSelect->menu() - SELECT_COLOR_ID);
  VColorSelect *target = m_colorSelects + colorIndex;
  // if the target of the event is our currently selected colorselect 
  // then unselect it because they right clicked a selected colorselect
  // otherwise they right clicked an existing color we need to clear it
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    colSelect->setSelected(false);
    colSelect->setActive(false);
    return;
  }
  Colorset newSet;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.getColorset(LED_MULTI, newSet);
  } else {
    m_vortex.getColorset((LedPos)sels[0], newSet);
  }
  // if the color select was made inactive
  if (!target->isActive()) {
    debug("Disabled color slot %u", colorIndex);
    newSet.removeColor(colorIndex);
  } else {
    debug("Updating color slot %u", colorIndex);
    newSet.set(colorIndex, colSelect->getColor()); // getRawColor?
  }
  // check whether the event is a left click or not, if left click only
  // update the colorset if a new entry was added
  if (sevent != VColorSelect::SelectEvent::SELECT_RIGHT_CLICK) {
    // when ther user clicks and empty color that is further than
    // the next available slot it will auto shift up, we need to
    // unselect the target colorselect they picked in that case
    if (newSet.numColors() <= colorIndex) {
      target->setSelected(false);
    }
    if (sevent == VColorSelect::SelectEvent::SELECT_LEFT_CLICK) {
      for (uint32_t i = 0; i < 8; ++i) {
        if (i == colorIndex) {
          continue;
        }
        if (m_colorSelects[i].isSelected()) {
          m_colorSelects[i].setSelected(false);
        }
      }
      m_colorSelects[colorIndex].setSelected(true);
    }
    if (sevent == VColorSelect::SelectEvent::SELECT_CTRL_LEFT_CLICK) {
      // do nothing just select it
    }
    if (sevent == VColorSelect::SelectEvent::SELECT_SHIFT_LEFT_CLICK) {
      // select all in between
      if (m_colorSelects[m_lastClickedColor].isSelected()) {
        uint32_t lower = (m_lastClickedColor < colorIndex) ? m_lastClickedColor : colorIndex;
        uint32_t higher = (m_lastClickedColor >= colorIndex) ? m_lastClickedColor : colorIndex;
        for (uint32_t i = lower; i <= higher; i++) {
          m_colorSelects[i].setActive(true);
          m_colorSelects[i].setSelected(true);
          m_colorSelects[i].redraw();
          if (i >= newSet.numColors()) {
            newSet.set(i, colSelect->getColor()); // getRawColor?
          }
        }
      }
    }
    if (!m_colorPicker.isOpen()) {
      m_colorPicker.show();
    }
    uint32_t rawCol = m_colorSelects[colorIndex].getColor();
    m_colorPicker.setColor(RGBColor(rawCol));
    m_colorPicker.refreshColor();
    m_colorPicker.pickCol();
    m_lastClickedColor = colorIndex;
  }
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    m_vortex.setColorset(LED_MULTI, newSet);
  } else {
    for (uint32_t i = 0; i < sels.size(); ++i) {
      m_vortex.setColorset((LedPos)sels[i], newSet);
    }
  }
  refreshModeList();
  // update the demo
  demoCurMode();
}

void VortexEditor::demoColor(uint32_t rawCol)
{
  VortexPort *port = nullptr;
  if (!isConnected() || !getCurPort(&port)) {
    return;
  }
  // now immediately tell it what to do
  port->writeData(EDITOR_VERB_DEMO_MODE);
  // read data again
  port->expectData(EDITOR_VERB_READY);
  // now unserialize the stream of data that was read
  ByteStream curMode;
  PatternArgs args(1, 0, 0);
  Colorset newSet(rawCol);
  Mode tmpMode(m_engine, PATTERN_STROBE, &args, &newSet);
  tmpMode.init();
  tmpMode.saveToBuffer(curMode);
  // send, the, mode
  port->writeData(curMode);
  // wait for the done response
  port->expectData(EDITOR_VERB_DEMO_MODE_ACK);
  string modeName = "Mode_" + to_string(m_vortex.curModeIndex()) + "_" + m_vortex.getModeName();
  // Set status? maybe soon
  //m_statusBar.setStatus(RGB(0, 255, 255), ("Demoing " + modeName).c_str());
}

void VortexEditor::refreshAll()
{
  refreshPortList();
  refreshModeList(true);
}

// refresh the port list
void VortexEditor::refreshPortList()
{
  VortexPort *selectedPort = nullptr;
  getCurPort(&selectedPort);
  // perform refresh as normal
  m_portSelection.clearItems();
  for (auto port = m_portList.begin(); port != m_portList.end(); ++port) {
    // if the port isn't active don't show it yet
    if (!port->second->isActive()) {
      continue;
    }
    m_portSelection.addItem("Port " + to_string(port->first));
  }
  if (selectedPort && selectedPort->isActive()) {
    for (uint32_t i = 0; i < m_portList.size(); ++i) {
      if (m_portList[i].first == selectedPort->port().portNumber()) {
        m_portSelection.setSelection(i);
      }
    }
  }
  // hack: for now just refresh status here
  refreshStatus();
  refreshStorageBar();
}

void VortexEditor::refreshStatus()
{
  int sel = getPortListIndex();
  if (sel < 0 || !m_portList.size()) {
    m_statusBar.setStatus(RGB(255, 0, 0), "Disconnected");
    return;
  }
  if (!isConnected()) {
    m_statusBar.setStatus(RGB(255, 0, 0), "Disconnected");
    return;
  }
  m_statusBar.setStatus(RGB(0, 255, 0), "Connected");
}

void VortexEditor::refreshStorageBar()
{
  uint32_t total = 0;
  uint32_t used = 0;
  m_vortex.getStorageStats(&total, &used);
  float percent = (float)used / (float)total;
  // integer percent from 0 - 280
  uint32_t intPct = (uint32_t)(percent * 280.0);
  m_storageProgress.setSelection(intPct, 0);
  m_storageProgress.redraw();
  static HBITMAP bitty = nullptr;
  if (bitty) {
    DeleteObject(bitty);
  }
  bitty = genProgressBack(280, 16, percent);
  m_storageProgress.setBackground(bitty);
  string space = "Storage Space (";
  space += to_string(used);
  space += " / ";
  space += to_string(total);
  space += ")";
  m_storageProgress.setTooltip(space);
}

HBITMAP VortexEditor::genProgressBack(uint32_t width, uint32_t height, float progress)
{
  COLORREF *cols = new COLORREF[width * height];
  if (!cols) {
    return nullptr;
  }
  uint32_t threshold = (uint32_t)(width * progress);
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t x = 0; x < width; ++x) {
    RGBColor rgbCol;
    COLORREF col = 0;
    if (x < threshold) {
      uint32_t r = (uint32_t)(progress * 255);
      // scale green to 512 to make it stay green longer
      uint32_t g = (uint32_t)((1.0f - progress) * 512);
      if (g > 255) {
        g = 255;
      }
      col = (r << 16) | (g << 8);
    }
    for (uint32_t y = 0; y < height; ++y) {
      cols[(y * width) + x] = col;
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

  // get the current pattern selection from the pattern dropdown
PatternID VortexEditor::patternSelection() const
{
  return (PatternID)(m_patternSelectComboBox.getSelection() - 1);
}

void VortexEditor::refreshModeList(bool recursive)
{
  m_modeListBox.clearItems();
  int curSel = (int)m_vortex.curModeIndex();
  // We have to actually iterate the modes with nextmode because Vortex can't just
  // instantiate one and return it which is kinda dumb but just how it works for now
  m_vortex.setCurMode(0, false);
  for (uint32_t i = 0; i < m_vortex.numModes(); ++i) {
    // just use the pattern name from the first pattern
    string modeName = "Mode " + to_string(i) + " (" + m_vortex.getModeName() + ")";
    m_modeListBox.addItem(modeName);
    // go to next mode
    m_vortex.nextMode(false);
  }
  // restore the selection
  m_modeListBox.setSelection(curSel);
  m_vortex.setCurMode(curSel, false);
  if (recursive) {
    refreshLedList(recursive);
  }
  // hack: for now just refresh status here
  refreshStatus();
  refreshStorageBar();
}

void VortexEditor::refreshLedList(bool recursive)
{
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  m_ledsMultiListBox.clearItems();
  if (m_vortex.getPatternID(LED_MULTI) == PATTERN_NONE) {
    for (LedPos pos = LED_FIRST; pos < m_vortex.numLedsInMode(); ++pos) {
      string ledName = m_vortex.ledToString(pos) + " (" + m_vortex.getPatternName(pos) + ")";
      m_ledsMultiListBox.addItem(ledName);
    }
  } else {
    string ledName = "Multi led (" + m_vortex.getPatternName(LED_MULTI) + ")";
    m_ledsMultiListBox.addItem(ledName);
    // TODO: support both rendering multi and single at same time... not for now
  }
  // restore the selection
  m_ledsMultiListBox.setSelections(sels);
  if (recursive) {
    refreshPatternSelect(recursive);
    refreshColorSelect(recursive);
    refreshParams(recursive);
  }
}

void VortexEditor::refreshPatternSelect(bool recursive)
{
  if (!m_ledsMultiListBox.numItems() || !m_ledsMultiListBox.numSelections()) {
    m_patternSelectComboBox.setSelection(-1);
    m_patternSelectComboBox.setEnabled(false);
    return;
  }
  m_patternSelectComboBox.setEnabled(true);
  int sel = m_ledsMultiListBox.getSelection();
  if (sel < 0) {
    // still necessary?
    m_patternSelectComboBox.setSelection(PATTERN_NONE);
    return;
  }
  m_patternSelectComboBox.clearItems();
  // whether to allow multi-led patterns to appear in the dropdown
  bool allow_multi = (sel == 0);
  // whether we are currently selecting a multi
  if (m_vortex.isCurModeMulti()) {
    // so that we can use getPatternID(sel)
    sel = LED_MULTI;
  }
  // get the pattern
  for (PatternID id = PATTERN_NONE; id < PATTERN_COUNT; ++id) {
    bool isMulti = isMultiLedPatternID(id);
    if (!allow_multi && isMulti) {
      continue;
    }
    string patternName = m_vortex.patternToString(id);
    if (isMulti) {
      patternName += " *";
    }
    m_patternSelectComboBox.addItem(patternName);
    // TODO: put multi-led in a separate position in UI so this is more elegant
    if (id == m_vortex.getPatternID((LedPos)sel)) {
      m_patternSelectComboBox.setSelection(id + 1);
    }
  }
}

void VortexEditor::refreshColorSelect(bool recursive)
{
  if (!m_ledsMultiListBox.numItems() || !m_ledsMultiListBox.numSelections()) {
    for (uint32_t i = 0; i < 8; ++i) {
      m_colorSelects[i].clear();
      m_colorSelects[i].setActive(false);
    }
    return;
  }
  int pos = m_ledsMultiListBox.getSelection();
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
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getColorset((LedPos)pos, set);
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
  PatternID sel = patternSelection();
  int pos = m_ledsMultiListBox.getSelection();
  if (pos < 0) {
    for (uint32_t i = 0; i < 8; ++i) {
      m_paramTextBoxes[i].clearText();
      m_paramTextBoxes[i].setEnabled(false);
      m_paramTextBoxes[i].setVisible(false);
    }
    return;
  }
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // disable all edit boxes but don't change their text, sorry.
    for (uint32_t i = 0; i < 8; ++i) {
      m_paramTextBoxes[i].setEnabled(false);
      m_paramTextBoxes[i].setVisible(false);
    }
    return;
  }
  vector<string> tips = m_vortex.getCustomParams(sel);
  if (sels.size() > 1) {
    bool all_same = true;
    PatternID base = m_vortex.getPatternID((LedPos)sels[0]);
    for (uint32_t i = 1; i < sels.size(); ++i) {
      if (m_vortex.getPatternID((LedPos)sels[i]) != base) {
        all_same = false;
      }
    }
    if (!all_same) {
      // disable all edit boxes but don't change their text, sorry.
      for (uint32_t i = 0; i < 8; ++i) {
        m_paramTextBoxes[i].setEnabled(false);
        m_paramTextBoxes[i].setVisible(false);
      }
      return;
    }
  }
  PatternArgs args;
  // TODO: put multi-led in a separate position in UI so this is more elegant
  if (m_vortex.isCurModeMulti()) {
    pos = LED_MULTI;
  }
  m_vortex.getPatternArgs((LedPos)pos, args);
  uint8_t *pArgs = (uint8_t *)&args.arg1;
  // get the number of params for the current pattern selection
  uint32_t numParams = m_vortex.numCustomParams(sel);
  // iterate all active params and activate
  for (uint32_t i = 0; i < numParams; ++i) {
    m_paramTextBoxes[i].setText(to_string(pArgs[i]).c_str(), false);
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
}

bool VortexEditor::isConnected()
{
  VortexPort *port = nullptr;
  if (!getCurPort(&port)) {
    return false;
  }
  if (!port->isConnected()) {
    return false;
  }
  // try to begin each time we check for connection just in case the glove reset
  // then it will have sent it's hello and we need to capture it again
  if (!port->tryBegin()) {
    return false;
  }
  return port->isActive();
}

bool VortexEditor::isPortConnected(uint32_t portNum) const
{
  if (!m_portList.size()) {
    return false;
  }
  for (uint32_t i = 0; i < m_portList.size(); ++i) {
    if (m_portList[i].first == portNum) {
      return m_portList[i].second->isConnected();
    }
  }
  return false;
}

bool VortexEditor::getCurPort(VortexPort **outPort)
{
  if (!m_portList.size()) {
    return false;
  }
  int sel = getPortListIndex();
  if (sel < 0) {
    return false;
  }
  if (!m_portList.size()) {
    return false;
  }
  *outPort = m_portList[sel].second.get();
  return (*outPort != nullptr);
}

uint32_t VortexEditor::getPortID() const
{
  string text = m_portSelection.getSelectionText();
  return strtoul(text.c_str() + 5, NULL, 10);
}

int VortexEditor::getPortListIndex() const
{
  if (m_portSelection.getSelection() == -1) {
    return -1;
  }
  uint32_t id = getPortID();
  for (uint32_t i = 0; i < m_portList.size(); ++i) {
    if (m_portList[i].first == id) {
      return i;
    }
  }
  return -1;
}

void VortexEditor::beginTutorial()
{
  m_tutorial.show();
}
