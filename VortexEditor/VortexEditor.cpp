#include "VortexEditor.h"
#include "EditorConfig.h"

#include "ArduinoSerial.h"

#include "GUI/VWindow.h"

#include "resource.h"

#include <string>

using namespace std;

VortexEditor *g_pEditor = nullptr;

VortexEditor::VortexEditor() :
  m_hInstance(NULL),
  m_ports(),
  m_window(),
  m_pushButton(),
  m_pullButton(),
  m_loadButton(),
  m_saveButton(),
  m_portSelection(),
  m_consoleHandle(nullptr)
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

  if (!m_consoleHandle) {
    AllocConsole();
    freopen_s(&m_consoleHandle, "CONOUT$", "w", stdout);
  }

  m_hInstance = hInstance;

  // initialize the window accordingly
  m_window.init(hInstance, EDITOR_TITLE, EDITOR_BACK_COL, EDITOR_WIDTH, EDITOR_HEIGHT, g_pEditor);
  m_portSelection.init(hInstance, m_window, "Select Port", EDITOR_BACK_COL, 150, 300, 16, 16, 0, selectPortCallback);
  m_connectButton.init(hInstance, m_window, "Connect", EDITOR_BACK_COL, 72, 28, 16, 48, ID_FILE_CONNECT, connectCallback);
  m_pushButton.init(hInstance, m_window, "Push", EDITOR_BACK_COL, 72, 28, 16, 80, ID_FILE_PUSH, pushCallback);
  m_pullButton.init(hInstance, m_window, "Pull", EDITOR_BACK_COL, 72, 28, 16, 112, ID_FILE_PULL, pullCallback);
  m_loadButton.init(hInstance, m_window, "Load", EDITOR_BACK_COL, 72, 28, 16, 144, ID_FILE_LOAD, loadCallback);
  m_saveButton.init(hInstance, m_window, "Save", EDITOR_BACK_COL, 72, 28, 16, 176, ID_FILE_SAVE, saveCallback);

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

void VortexEditor::connect()
{
  string data = readPort(m_portSelection.getSelection());
  if (data[0] == '=' && data[1] == '=') {
    if (strncmp(data.c_str() + 3, "Vortex Framework v", sizeof("Vortex Framework v") - 1) == 0) {
      printf("Connected to Vortex Gloveset\n");
    }
    writePort(m_portSelection.getSelection(), "Hello");
  }
  while (1) {
    data = readPort(m_portSelection.getSelection());
    if (data.size()) {
      printf("Read: [%s]\n", data.c_str());
    }
  }
}

void VortexEditor::push()
{
}

void VortexEditor::pull()
{
}

void VortexEditor::load()
{
}

void VortexEditor::save()
{
}

void VortexEditor::selectPort()
{
}

void VortexEditor::scanPorts()
{
  for (uint32_t i = 0; i < 255; ++i) {
    string port = "\\\\.\\COM" + to_string(i);
    ArduinoSerial serialPort(port);
    if (serialPort.IsConnected()) {
      m_ports.push_back(make_pair(i, move(serialPort)));
    }
  }
  for (auto port = m_ports.begin(); port != m_ports.end(); ++port) {
    m_portSelection.addItem(to_string(port->first));
    printf("Connected port %u\n", port->first);
  }
}

std::string VortexEditor::readPort(uint32_t portIndex)
{
  if (portIndex >= m_ports.size()) {
    return "";
  }
  ArduinoSerial *serial = &m_ports[portIndex].second;
  // read with NULL args to get expected amount
  int32_t amt = serial->ReadData(NULL, 0);
  if (amt == -1 || amt == 0) {
    // no data to read
    return "";
  }
  // allocate buffer for amount
  char *buf = (char *)calloc(1, amt + 1);
  if (!buf) {
    // ???
    return "";
  }
  // read the data into the buffer
  serial->ReadData(buf, amt);
  // just print the buffer
  printf("Data on port %u: [%s]\n", m_ports[portIndex].first, buf);
  string result = buf;
  free(buf);
  return result;
}

void VortexEditor::writePort(uint32_t portIndex, std::string data)
{
  if (portIndex >= m_ports.size()) {
    return;
  }
  ArduinoSerial *serial = &m_ports[portIndex].second;
  // read the data into the buffer
  serial->WriteData(data.c_str(), data.size());
  // just print the buffer
  printf("Wrote to port %u: [%s]\n", m_ports[portIndex].first, data.c_str());
}

void VortexEditor::connectCallback(void *editor)
{
  ((VortexEditor *)editor)->connect();
}

void VortexEditor::pushCallback(void *editor)
{
  ((VortexEditor *)editor)->push();
}

void VortexEditor::pullCallback(void *editor)
{
  ((VortexEditor *)editor)->pull();
}

void VortexEditor::loadCallback(void *editor)
{
  ((VortexEditor *)editor)->load();
}

void VortexEditor::saveCallback(void *editor)
{
  ((VortexEditor *)editor)->save();
}

void VortexEditor::selectPortCallback(void *editor)
{
  ((VortexEditor *)editor)->selectPort();
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
