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
#include "GUI/VStatusBar.h"
#include "GUI/VComboBox.h"
#include "GUI/VListBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VWindow.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

// editor includes
#include "ArduinoSerial.h"

// stl includes
#include <memory>
#include <vector>

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
public:
  VortexEditor();
  ~VortexEditor();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the test framework
  void run();

  HINSTANCE hInst() const { return m_hInstance; }

private:
  class VortexPort {
  public:
    VortexPort();
    VortexPort(ArduinoSerial &&serial);
    VortexPort(VortexPort &&other) noexcept;
    ~VortexPort();
    void operator=(VortexPort &&other) noexcept;
    bool begin();
    void listen();
    bool isConnected() const;
    bool isActive() const;
    ArduinoSerial &port();
    // set whether active or not
    void setActive(bool active);
    // amount of data ready
    int bytesAvailable();
    // read out any available data
    int readData(ByteStream &stream);
    // wait till data arrives then read it out
    int waitData(ByteStream &stream);
    // write a message to the port
    int writeData(const std::string &message);
    // write a buffer of binary data to the port
    int writeData(ByteStream &stream);
    // wait for some data
    bool expectData(const std::string &data);
    // read data in a loop
    void readInLoop(ByteStream &outStream);
    // helper to validate a handshake message
    bool parseHandshake(const ByteStream &handshakewindow);
    // read out the full list of modes
    bool readModes(ByteStream &outModes);
  private:
    // the raw serial connection
    ArduinoSerial m_serialPort;
    // a handle to the thread that waits for the initial handshake
    HANDLE m_hThread;
    // whether the port is 'active' ie the handshake has been received
    bool m_portActive;
    // thread func to wait and begin a port connection
    static DWORD __stdcall beginPort(void *ptr);
  };

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
  static void selectColorCallback(void *editor, VWindow *window)   { ((VortexEditor *)editor)->selectColor(window); }
  static void paramEditCallback(void *editor, VWindow *window)     { ((VortexEditor *)editor)->paramEdit(window); }

  // menu handler
  static void handleMenusCallback(void *editor, uintptr_t hMenu)   { ((VortexEditor *)editor)->handleMenus(hMenu); }

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
  void selectColor(VWindow *window);
  void paramEdit(VWindow *window);

  // callback to handle menus
  void handleMenus(uintptr_t hMenu);

  // callback to handle device notifications
  void deviceChange(DEV_BROADCAST_HDR *dbh, bool added);

  // helper for color changer menus
  void applyColorset(const Colorset &set, const std::vector<int> &selections);
  void applyPattern(PatternID id, const std::vector<int> &selections);
  void applyColorsetToAll(const Colorset &set);
  void applyPatternToAll(PatternID id);
  void copyColorset();
  void pasteColorset();
  void copyLED();
  void pasteLED();

  // helper for clipboard
  void getClipboard(std::string &clipData);
  void setClipboard(const std::string &clipData);

  // refresh the various UI elements
  void refreshPortList();
  void refreshStatus();
  void refreshModeList(bool recursive = true);
  void refreshFingerList(bool recursive = true);
  void refreshPatternSelect(bool recursive = true);
  void refreshColorSelect(bool recursive = true);
  void refreshParams(bool recursive = true);

  // whether connected to gloveset
  bool isConnected();
  bool isPortConnected(uint32_t port) const;
  bool getCurPort(VortexEditor::VortexPort **outPort);

  uint32_t getPortID() const;
  int getPortListIndex() const;

  // helper to split strings
  void splitString(const std::string &str, std::vector<std::string> &splits, char letter);

  // ==================================
  //  Member data

  // main instance
  HINSTANCE m_hInstance;
  // Console handle for debugging
  FILE *m_consoleHandle;
  // list of ports
  std::vector<std::pair<uint32_t, std::unique_ptr<VortexPort>>> m_portList;
  // accelerator table for hotkeys
  HACCEL m_accelTable;

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
  // the list of fingers is a multi select
  VMultiListBox m_ledsMultiListBox;
  // the pattern selection
  VComboBox m_patternSelectComboBox;
  // color select options
  VColorSelect m_colorSelects[8];
  // parameters text boxes, there's 8 params
  VTextBox m_paramTextBoxes[8];
};

extern VortexEditor *g_pEditor;
