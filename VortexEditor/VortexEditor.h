#pragma once

// windows includes
#include <windows.h>

// arduino includes
#include "Patterns/Patterns.h"
#include "Leds/LedTypes.h"

// gui includes
#include "GUI/VWindow.h"
#include "GUI/VButton.h"
#include "GUI/VComboBox.h"
#include "GUI/VListBox.h"

// editor includes
#include "ArduinoSerial.h"

// stl includes
#include <vector>

class ByteStream;
class Mode;

class VortexEditor
{
public:
  VortexEditor();
  ~VortexEditor();

  // initialize the test framework
  bool init(HINSTANCE hInstance);
  // run the test framework
  void run();

  // print to the log
  void printlog(const char *file, const char *func, int line, const char *msg, va_list list);

private:
  // callbacks for actions
  void connect();
  void push();
  void pull();
  void load();
  void save();
  void selectPort();
  void selectMode();
  void addMode();
  void delMode();
  void selectFinger();
  void selectPattern();

  // special handler that is called after each action
  void waitIdle();

  // callbacks wrappers so that the callback handlers of
  // the gui elements can call a static routine
  static void connectCallback(void *editor)       { ((VortexEditor *)editor)->connect(); }
  static void pushCallback(void *editor)          { ((VortexEditor *)editor)->push(); }
  static void pullCallback(void *editor)          { ((VortexEditor *)editor)->pull(); }
  static void loadCallback(void *editor)          { ((VortexEditor *)editor)->load(); }
  static void saveCallback(void *editor)          { ((VortexEditor *)editor)->save(); }
  static void selectPortCallback(void *editor)    { ((VortexEditor *)editor)->selectPort(); }
  static void selectModeCallback(void *editor)    { ((VortexEditor *)editor)->selectMode(); }
  static void addModeCallback(void *editor)       { ((VortexEditor *)editor)->addMode(); }
  static void delModeCallback(void *editor)       { ((VortexEditor *)editor)->delMode(); }
  static void selectFingerCallback(void *editor)  { ((VortexEditor *)editor)->selectFinger(); }
  static void selectPatternCallback(void *editor) { ((VortexEditor *)editor)->selectPattern(); }

  bool validateHandshake(const ByteStream &handshake);

  // refresh the mode list
  void refreshModeList();
  void refreshFingerList();

  // various other actions
  void scanPorts();
  bool readPort(uint32_t port, ByteStream &outStream);
  bool readModes(uint32_t portIndex, ByteStream &outModes);
  void readInLoop(uint32_t port, ByteStream &outStream);
  void writePortRaw(uint32_t portIndex, const uint8_t *data, size_t size);
  void writePort(uint32_t portIndex, const ByteStream &data);
  void writePort(uint32_t port, std::string data);

  std::string getPatternName(PatternID id) const;
  std::string getLedName(LedPos pos) const;

  // ==================================
  //  Member data

  // main instance
  HINSTANCE m_hInstance;

  // Console handle for debugging
  FILE *m_consoleHandle;

  // map of ports
  std::vector<std::pair<uint32_t, ArduinoSerial>> m_portList;

  // ==================================
  //  GUI Members

  // main window
  VWindow m_window;
  // the list of com ports
  VComboBox m_portSelection;
  // the four buttons
  VButton m_connectButton;
  VButton m_pushButton;
  VButton m_pullButton;
  VButton m_loadButton;
  VButton m_saveButton;
  // the list of modes
  VListBox m_modeListBox;
  // the add/remove mode button
  VButton m_addModeButton;
  VButton m_delModeButton;
  // the list of fingers
  VListBox m_fingersListBox;
  // the pattern selection
  VComboBox m_patternSelectComboBox;
};

extern VortexEditor *g_pEditor;
