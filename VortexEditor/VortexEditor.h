#pragma once

// windows includes
#include <windows.h>

// arduino includes
#include "Patterns/Patterns.h"
#include "Leds/LedTypes.h"

// gui includes
#include "GUI/VColorSelect.h"
#include "GUI/VComboBox.h"
#include "GUI/VListBox.h"
#include "GUI/VTextBox.h"
#include "GUI/VWindow.h"
#include "GUI/VButton.h"
#include "GUI/VLabel.h"

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
  // callbacks wrappers so that the callback handlers of
  // the gui elements can call a static routine
  static void selectPortCallback(void *editor, VWindow *window)    { ((VortexEditor *)editor)->selectPort(window); }
  static void refreshCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->refresh(window); }
  static void connectCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->connect(window); }
  static void pushCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->push(window); }
  static void pullCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->pull(window); }
  static void loadCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->load(window); }
  static void saveCallback(void *editor, VWindow *window)          { ((VortexEditor *)editor)->save(window); }
  static void selectModeCallback(void *editor, VWindow *window)    { ((VortexEditor *)editor)->selectMode(window); }
  static void addModeCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->addMode(window); }
  static void delModeCallback(void *editor, VWindow *window)       { ((VortexEditor *)editor)->delMode(window); }
  static void selectFingerCallback(void *editor, VWindow *window)  { ((VortexEditor *)editor)->selectFinger(window); }
  static void selectPatternCallback(void *editor, VWindow *window) { ((VortexEditor *)editor)->selectPattern(window); }
  static void selectColorCallback(void *editor, VWindow *window)   { ((VortexEditor *)editor)->selectColor(window); }
  static void paramEditCallback(void *editor, VWindow *window)     { ((VortexEditor *)editor)->paramEdit(window); }

  // callbacks for actions
  void selectPort(VWindow *window);
  void refresh(VWindow *window);
  void connect(VWindow *window);
  void push(VWindow *window);
  void pull(VWindow *window);
  void load(VWindow *window);
  void save(VWindow *window);
  void selectMode(VWindow *window);
  void addMode(VWindow *window);
  void delMode(VWindow *window);
  void selectFinger(VWindow *window);
  void selectPattern(VWindow *window);
  void selectColor(VWindow *window);
  void paramEdit(VWindow *window);

  // special handler that is called after each action
  void waitIdle();

  bool validateHandshake(const ByteStream &handshakewindow);

  // refresh the mode list
  void refreshModeList();
  void refreshFingerList();
  void refreshPatternSelect();
  void refreshColorSelect();
  void refreshParams();

  // various other actions
  bool readPort(uint32_t port, ByteStream &outStream);
  bool readModes(uint32_t portIndex, ByteStream &outModes);
  void readInLoop(uint32_t port, ByteStream &outStream);
  void writePortRaw(uint32_t portIndex, const uint8_t *data, size_t size);
  void writePort(uint32_t portIndex, const ByteStream &data);
  void writePort(uint32_t port, std::string data);

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
  // the various buttons
  VButton m_refreshButton;
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
  // color select options
  VColorSelect m_colorSelects[8];
  // parameters text boxes, there's 8 params
  VTextBox m_paramTextBoxes[8];
};

extern VortexEditor *g_pEditor;
