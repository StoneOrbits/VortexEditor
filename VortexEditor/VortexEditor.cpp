#include "VortexEditor.h"

// VortexEngine includes
#include "VortexEngine.h"
#include "Serial/ByteStream.h"
#include "EditorConfig.h"
#include "Modes/Modes.h"

// Editor includes
#include "ArduinoSerial.h"
#include "GUI/VWindow.h"
#include "resource.h"

// stl includes
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

  // init the engine
  VortexEngine::init();
  // clear the modes
  Modes::clearModes();

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
  ByteStream stream;
  // try to read the handshake
  if (!readPort(m_portSelection.getSelection(), stream)) {
    // failure
    return;
  }
  if (!validateHandshake(stream)) {
    // failure
    return;
  }
  printf("Connected to Vortex Gloveset\n");
  writePort(m_portSelection.getSelection(), "Hello");
  bool readMode = false;
  while (1) {
    stream.clear();
    if (!readPort(m_portSelection.getSelection(), stream)) {
      // error?
      continue;
    }
    if (!stream.size()) {
      continue;
    }
    break;
  }
  if (!Modes::unserialize(stream)) {
    printf("Unserialize failed\n");
  }
  printf("Unserialized %u modes\n", Modes::numModes());
  writePort(m_portSelection.getSelection(), "Thanks");
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

bool VortexEditor::readPort(uint32_t portIndex, ByteStream &outStream)
{
  if (portIndex >= m_ports.size()) {
    return false;
  }
  ArduinoSerial *serial = &m_ports[portIndex].second;
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
  printf("Data on port %u: [%s] (%u bytes)\n", m_ports[portIndex].first, outStream.data(), amt);
  return true;
}

void VortexEditor::writePort(uint32_t portIndex, std::string data)
{
  if (portIndex >= m_ports.size()) {
    return;
  }
  ArduinoSerial *serial = &m_ports[portIndex].second;
  // write the data into the serial port
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
