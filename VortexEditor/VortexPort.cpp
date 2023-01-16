#include "VortexPort.h"

#include "VortexEditor.h"

#include "Serial/ByteStream.h"

using namespace std;

VortexPort::VortexPort() :
  m_serialPort(),
  m_hThread(nullptr),
  m_portActive(false)
{
}

VortexPort::VortexPort(ArduinoSerial &&serial) :
  m_serialPort(std::move(serial)),
  m_hThread(nullptr),
  m_portActive(false)
{
}

VortexPort::VortexPort(VortexPort &&other) noexcept :
  VortexPort()
{
  *this = std::move(other);
}

VortexPort::~VortexPort()
{
  if (m_hThread) {
    TerminateThread(m_hThread, 0);
    CloseHandle(m_hThread);
  }
}

void VortexPort::operator=(VortexPort &&other) noexcept
{
  m_serialPort = std::move(other.m_serialPort);
  m_portActive = other.m_portActive;
  m_hThread = other.m_hThread;

  other.m_portActive = false;
  other.m_hThread = nullptr;
}

bool VortexPort::begin()
{
  ByteStream stream;
  if (!isConnected()) {
    setActive(false);
    return false;
  }
  // try to read the handshake
  if (!readData(stream)) {
    // no handshake or goodbye so just return whether currently active
    return m_portActive;
  }
  // validate the handshake is correct
  if (!parseHandshake(stream)) {
    // failure
    return false;
  }
  setActive(true);
  return true;
}

void VortexPort::listen()
{
  m_hThread = CreateThread(NULL, 0, beginPort, this, 0, NULL);
}

bool VortexPort::isConnected() const
{
  return m_serialPort.isConnected();
}

bool VortexPort::isActive() const
{
  return m_portActive;
}

ArduinoSerial &VortexPort::port()
{
  return m_serialPort;
}

void VortexPort::setActive(bool active)
{
  m_portActive = active;
}

// amount of data ready
int VortexPort::bytesAvailable()
{
  return m_serialPort.bytesAvailable();
}

int VortexPort::readData(ByteStream &stream)
{
  uint32_t avail = bytesAvailable();
  // if there's data already just extend, otherwise init
  if (!avail || !stream.extend(avail)) {
    return 0;
  }
  uint32_t amt = m_serialPort.readData((void *)(stream.data() + stream.size()), avail);
  // hack to increase ByteStream size
  **(uint32_t **)&stream += amt;
  return amt;
}

int VortexPort::waitData(ByteStream &stream)
{
  BOOL result;
  int len = 0;
  DWORD bytesRead = 0;
  uint8_t byte = 0;
  // Try to read 1 byte so we block till data arrives
  if (!m_serialPort.rawRead(&byte, 1)) {
    return 0;
  }
  // insert the byte into the output stream
  stream.serialize(byte);
  // check how much more data is avail
  int data = bytesAvailable();
  if (data > 0) {
    // read the rest of the data into the stream
    readData(stream);
  }
  return stream.size();
}

int VortexPort::writeData(const std::string &message)
{
  // just print the buffer
  //debug("Wrote to port %u: [%s]", m_serialPort.portNumber(), message.c_str());
  return m_serialPort.writeData(message.c_str(), message.size());
}

int VortexPort::writeData(ByteStream &stream)
{
  // write the data into the serial port
  uint32_t size = stream.rawSize();
  // create a new ByteStream that will contain the size + full stream
  ByteStream buf(size + sizeof(size));
  // serialize the size into the buf
  buf.serialize(size);
  // append the raw data of the input stream (crc/flags/size/buffer)
  buf.append(ByteStream(size, (const uint8_t *)stream.rawData()));
  // We must send the whole buffer in one go, cannot send size first
  // NOTE: when I sent this in two sends it would actually cause the arduino
  // to only receive the size and not the buffer. It worked fine in the test
  // framework but not for arduino serial. So warning, always send in one chunk.
  // Even when I flushed the file buffers it didn't fix it.
  m_serialPort.writeData(buf.data(), buf.size());
  //debug("Wrote %u bytes of raw data", size);
  return buf.size();
}

bool VortexPort::expectData(const std::string &data)
{
  ByteStream stream;
  readInLoop(stream);
  if (stream.size() < data.size()) {
    return false;
  }
  if (data != (char *)stream.data()) {
    return false;
  }
  return true;
}

void VortexPort::readInLoop(ByteStream &outStream)
{
  outStream.clear();
  // TODO: proper timeout lol
  while (1) {
    if (!readData(outStream)) {
      // error?
      continue;
    }
    if (!outStream.size()) {
      continue;
    }
    break;
  }
}

bool VortexPort::parseHandshake(const ByteStream &handshake)
{
  string handshakeStr = (char *)handshake.data();
  // if there is a goodbye message then the gloveset just left the editor
  // menu and we cannot send it messages anymore
  if (handshakeStr.find(EDITOR_VERB_GOODBYE) == (handshakeStr.size() - (sizeof(EDITOR_VERB_GOODBYE) - 1))) {
    setActive(false);
    g_pEditor->triggerRefresh();
    listen();
    return false;
  }
  // check the handshake for valid data
  if (handshakeStr.size() < 10) {
    //debug("Handshake size bad: %u", handshake.size());
    // bad handshake
    return false;
  }
  if (handshakeStr[0] != '=' || handshakeStr[1] != '=') {
    //debug("Handshake start bad: [%c%c]", handshakeStr[0], handshakeStr[1]);
    // bad handshake
    return false;
  }
  // TODO: improve handshake check
  string handshakeStart = handshakeStr.substr(0, 21);
  if (handshakeStart != "== Vortex Framework v") {
    //debug("Handshake data bad: [%s]", handshake.data());
    // bad handshake
    return false;
  }
  // looks good
  return true;
}

bool VortexPort::readModes(ByteStream &outModes)
{
  uint32_t size = 0;
  // first check how much is in the serial port
  int32_t amt = 0;
  // wait till amount available is enough
  while (m_serialPort.bytesAvailable() < sizeof(size));
  // read the size out of the serial port
  m_serialPort.readData((void *)&size, sizeof(size));
  if (!size || size > 4096) {
    //debug("Bad IR Data size: %u", size);
    return false;
  }
  // init outmodes so it's big enough
  outModes.init(size);
  uint32_t amtRead = 0;
  do {
    // read straight into the raw buffer, this will always have enough
    // space because outModes is big enough to hold the entire data
    uint8_t *readPos = ((uint8_t *)outModes.rawData()) + amtRead;
    amtRead += m_serialPort.readData((void *)readPos, size);
  } while (amtRead < size);
  return true;
}

DWORD __stdcall VortexPort::beginPort(void *ptr)
{
  VortexPort *port = (VortexPort *)ptr;
  while (!port->m_portActive) {
    ByteStream handshake;
    // wait for the handshake data indefinitely
    int32_t actual = port->waitData(handshake);
    if (!actual || !handshake.size()) {
      continue;
    }
    // validate it
    if (!port->parseHandshake(handshake)) {
      // failure
      continue;
    }
    //debug("Port %u active\n", port->m_serialPort.portNumber());
    port->m_portActive = true;
  }
  g_pEditor->triggerRefresh();
  // cleanup this thread this function is running in
  CloseHandle(port->m_hThread);
  port->m_hThread = nullptr;
  return 0;
}