#pragma once

// windows includes
#include <windows.h>
#include <Dbt.h>

// arduino includes
#include "Patterns/Patterns.h"
#include "Leds/LedTypes.h"

// gui includes
#include "GUI/VMultiListBox.h"
#include "GUI/VColorSelect.h"
#include "GUI/VChildwindow.h"
#include "GUI/VSelectBox.h"
#include "GUI/VStatusBar.h"
#include "GUI/VComboBox.h"
#include "GUI/VListBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VWindow.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

// engine includes
#include "VortexLib.h"

// editor includes
#include "VortexColorPicker.h"
#include "VortexModeRandomizer.h"
#include "VortexCommunityBrowser.h"
#include "VortexEditorTutorial.h"
#include "ArduinoSerial.h"

// stl includes
#include <memory>
#include <vector>

class VortexPort;
class ByteStream;
class Colorset;

// debug log
#ifdef _DEBUG
#define debug(msg, ...) printlog(__FILE__, __FUNCTION__, __LINE__, msg, __VA_ARGS__)
#else
#define debug(msg, ...)
#endif

class VortexEditor
{
  friend class VortexColorPicker;
  friend class VortexModeRandomizer;
  friend class VortexCommunityBrowser;
public:
  VortexEditor();
  ~VortexEditor();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the test framework
  void run();

  // send the refresh message to the window
  void triggerRefresh();

  HINSTANCE hInst() const { return m_hInstance; }

  void addMode(VWindow *window, const Mode *mode);

private:
  // print to the log
  static void printlog(const char *file, const char *func, int line, const char *msg, ...);

  // callbacks wrappers so that the callback handlers of
  // the gui elements can call a static routine
  static void selectPortCallback(void *editor, VWindow *window)    { ((VortexEditor *)editor)->selectPort(window); }
  static void pushCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->push(window); }
  static void pullCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->pull(window); }
  static void loadCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->load(window); }
  static void saveCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->save(window); }
  static void importCallback(void *editor, VWindow *window)        { ((VortexEditor *)editor)->importMode(window); }
  static void exportCallback(void *editor, VWindow *window)        { ((VortexEditor *)editor)->exportMode(window); }
  static void selectModeCallback(void *editor, VWindow *window)    { ((VortexEditor *)editor)->selectMode(window); }
  static void addModeCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->addMode(window); }
  static void delModeCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->delMode(window); }
  static void copyModeCallback(void *editor, VWindow *window)      { ((VortexEditor *)editor)->copyMode(window); }
  static void moveModeUpCallback(void *editor, VWindow *window)    { ((VortexEditor *)editor)->moveModeUp(window); }
  static void moveModeDownCallback(void *editor, VWindow *window)  { ((VortexEditor *)editor)->moveModeDown(window); }
  static void selectFingerCallback(void *editor, VWindow *window)  { ((VortexEditor *)editor)->selectFinger(window); }
  static void selectPatternCallback(void *editor, VWindow *window) { ((VortexEditor *)editor)->selectPattern(window); }
  static void copyToAllCallback(void *editor, VWindow *window)     { ((VortexEditor *)editor)->copyToAll(window); }
  static void paramEditCallback(void *editor, VWindow *window)     { ((VortexEditor *)editor)->paramEdit(window); }
  
  static void selectColorCallback(void *editor, VColorSelect *colSelect, VColorSelect::SelectEvent sevent)   { ((VortexEditor *)editor)->selectColor(colSelect, sevent); }

  // menu handler
  static void handleMenusCallback(void *editor, uintptr_t hMenu)   { ((VortexEditor *)editor)->handleMenus(hMenu); }

  // callback to refresh all uis
  static void refreshWindowCallback(void *editor, VWindow *window) { ((VortexEditor *)editor)->refreshAll(); }

  // connect test framework
  static void connectTestFrameworkCallback(void *editor, VWindow *window) { ((VortexEditor *)editor)->connectPort(0); }
  static void disconnectTestFrameworkCallback(void *editor, VWindow *window) { ((VortexEditor *)editor)->disconnectPort(0); }

  // device change handler
  static void deviceChangeCallback(void *editor, DEV_BROADCAST_HDR *dbh, bool added) { ((VortexEditor *)editor)->deviceChange(dbh, added); }

  // scan all ports for new connections
  void scanPorts();
  void connectPort(uint32_t portNum);
  void disconnectPort(uint32_t portNum);

  // callbacks for actions
  void selectPort(VWindow *window);
  void push(VWindow *window);
  void pull(VWindow *window);
  void load(VWindow *window);
  void save(VWindow *window);
  void importMode(VWindow *window);
  void exportMode(VWindow *window);
  void transmitVL(VWindow *window);
  void transmitIR(VWindow *window);
  void receiveVL(VWindow *window);
  void selectMode(VWindow *window);
  void demoCurMode();
  void clearDemo();
  void addMode(VWindow *window);
  void delMode(VWindow *window);
  void copyMode(VWindow *window);
  void moveModeUp(VWindow *window);
  void moveModeDown(VWindow *window);
  void selectFinger(VWindow *window);
  void selectPattern(VWindow *window);
  void copyToAll(VWindow *window);
  void paramEdit(VWindow *window);

  void selectColor(VColorSelect *colSelect, VColorSelect::SelectEvent sevent);

  // demo a color
  void demoColor(uint32_t rawCol);

  // callback to handle menus
  void handleMenus(uintptr_t hMenu);

  // callback to handle device notifications
  void deviceChange(DEV_BROADCAST_HDR *dbh, bool added);

  // helper for color changer menus
  void updateSelectedColors(uint32_t rawCol);
  void updateSelectedColor(VColorSelect *colSelect, uint32_t rawCol, bool demo = true);
  void applyColorset(const Colorset &set, const std::vector<int> &selections);
  void applyPattern(PatternID id, const std::vector<int> &selections);
  void applyColorsetToAll(const Colorset &set);
  void applyPatternToAll(PatternID id);
  void copyColorset();
  void pasteColorset();
  void copyLED();
  void pasteLED();
  void clearLED();

  // helper for clipboard
  void getClipboard(std::string &clipData);
  void setClipboard(const std::string &clipData);

  // refresh all UI elements
  void refreshAll();
  // port list and status are separate ui elements
  void refreshPortList();
  void refreshStatus();
  void refreshStorageBar();
  // but the mode list, led list, etc are all considered a heirarchy so
  // there is an option to recursively refresh all children elements
  void refreshModeList(bool recursive = true);
  void refreshLedList(bool recursive = true);
  void refreshPatternSelect(bool recursive = true);
  void refreshColorSelect(bool recursive = true);
  void refreshParams(bool recursive = true);

  // whether connected to gloveset
  bool isConnected();
  bool isPortConnected(uint32_t port) const;
  bool getCurPort(VortexPort **outPort);

  uint32_t getPortID() const;
  int getPortListIndex() const;

  // helper to split strings
  void splitString(const std::string &str, std::vector<std::string> &splits, char letter);

  // generate the progress bar background for storage space
  HBITMAP genProgressBack(uint32_t width, uint32_t height, float progress);

  // get the current pattern selection from the pattern dropdown
  PatternID patternSelection() const;

  // start the interactive tutorial
  void beginTutorial();

  // ==================================
  //  Member data

  // vortex lib
  Vortex m_vortex;
  // engine reference for LED_ constants
  VortexEngine &m_engine;

  // main instance
  HINSTANCE m_hInstance;
  // icon
  HICON m_hIcon;
  // Console handle for debugging
  FILE *m_consoleHandle;
  // list of ports
  std::vector<std::pair<uint32_t, std::unique_ptr<VortexPort>>> m_portList;
  // accelerator table for hotkeys
  HACCEL m_accelTable;
  // keeps track of the last colorset entry selected to support shift+click
  // which needs to set prevIndex to curIndex upon shift clicking
  uint32_t m_lastClickedColor;

  // ==================================
  //  GUI Members

  // main window
  VWindow m_window;
  // the list of com ports
  VComboBox m_portSelection;
  // the various buttons
  VButton m_pushButton;
  VButton m_pullButton;
  VStatusBar m_statusBar;
  // the list of modes
  VListBox m_modeListBox;
  // the add/remove mode button
  VButton m_addModeButton;
  VButton m_delModeButton;
  VButton m_copyModeButton;
  VButton m_moveModeUpButton;
  VButton m_moveModeDownButton;
  // the list of leds is a multi select
  VMultiListBox m_ledsMultiListBox;
  // the pattern selection
  VComboBox m_patternSelectComboBox;
  // color select options
  VColorSelect m_colorSelects[8];
  // parameters text boxes, there's 8 params
  VTextBox m_paramTextBoxes[8];
  // progress bar for storage
  VSelectBox m_storageProgress;

  // ==================================
  //  Sub-window GUIS

  // the vortex color picker window
  VortexColorPicker m_colorPicker;
  VortexModeRandomizer m_modeRandomizer;
  VortexCommunityBrowser m_communityBrowser;
  VortexEditorTutorial m_tutorial;
};

extern VortexEditor *g_pEditor;
