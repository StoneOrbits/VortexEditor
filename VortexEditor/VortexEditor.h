#pragma once

#include <windows.h>

#include "GUI/VWindow.h"
#include "GUI/VButton.h"
#include "GUI/VComboBox.h"

#include "ArduinoSerial.h"

#include <vector>

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
  // main instance
  HINSTANCE m_hInstance;

  // map of ports
  std::vector<std::pair<uint32_t, ArduinoSerial>> m_ports;

  // callbacks for actions
  void connect();
  void push();
  void pull();
  void load();
  void save();
  void selectPort();

  // various other actions
  void scanPorts();
  std::string readPort(uint32_t port);
  void writePort(uint32_t port, std::string data);

  // ==================================
  // GUI Members

  // main window
  VWindow m_window;
  // the four buttons
  VButton m_connectButton;
  VButton m_pushButton;
  VButton m_pullButton;
  VButton m_loadButton;
  VButton m_saveButton;
  // the list of com ports
  VComboBox m_portSelection;

  // Console handle for debugging
  FILE *m_consoleHandle;

  // callbacks wrappers
  static void connectCallback(void *arg);
  static void pushCallback(void *arg);
  static void pullCallback(void *arg);
  static void loadCallback(void *arg);
  static void saveCallback(void *arg);
  static void selectPortCallback(void *arg);
};

extern VortexEditor *g_pEditor;
