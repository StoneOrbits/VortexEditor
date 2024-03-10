#include "VortexPort.h"

#include "VortexEditor.h"

#include "Serial/ByteStream.h"

using namespace std;

uint32_t g_counter = 0;

// uncomment this to turn on send debug messages
//#define DEBUG_SENDING

#ifdef DEBUG_SENDING
#define debug_send(...) printf(__VA_ARGS__)
#else
#define debug_send(...)
#endif

VortexPort::VortexPort() :
  m_serialPort(),
  m_hThread(nullptr),
  m_portActive(false)
{
}

VortexPort::VortexPort(const std::string &portName) :
  m_serialPort(portName),
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
    m_hThread = nullptr;
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

bool VortexPort::tryBegin()
{
  //return true;
  if (m_hThread != nullptr) {
    // don't try because there's already a begin() thread waiting
    return false;
  }
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
  // do not spawn a thread if there is already one
  if (m_hThread) {
    return;
  }
  // spawn a new listener thread
  m_hThread = CreateThread(NULL, 0, begin, this, 0, NULL);
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
  debug_send("%u %x < Reading data %u\n", g_counter++, GetCurrentThreadId(), avail);
  uint32_t amt = m_serialPort.readData((void *)(stream.data() + stream.size()), avail);
  debug_send("%u %x << Read data %u: %s\n", g_counter++, GetCurrentThreadId(), amt, stream.data() + stream.size());
  // hack to increase ByteStream size
  **(uint32_t **)&stream += amt;
  return amt;
}

int VortexPort::waitData(ByteStream &stream)
{
  int len = 0;
  DWORD bytesRead = 0;
  uint8_t byte = 0;
  debug_send("%u %x < Waiting data\n", g_counter++, GetCurrentThreadId());
  // Try to read 1 byte so we block till data arrives
  if (!m_serialPort.rawRead(&byte, 1)) {
    return 0;
  }
  debug_send("%u %x << Waited 1 byte data\n", g_counter++, GetCurrentThreadId());
  // insert the byte into the output stream
  stream.serialize(byte);
  // check how much more data is avail
  int data = bytesAvailable();
  if (data > 0) {
    // read the rest of the data into the stream
    readData(stream);
  }
  debug_send("%u %x << Waited data: %s\n", g_counter++, GetCurrentThreadId(), stream.data());
  return stream.size();
}

int VortexPort::writeData(const std::string &message)
{
  debug_send("%u %x > Writing message: %s\n", g_counter++, GetCurrentThreadId(), message.c_str());
  // just print the buffer
  int rv = m_serialPort.writeData((uint8_t *)message.c_str(), message.size());
  debug_send("%u %x >> Wrote message: %s\n", g_counter++, GetCurrentThreadId(), message.c_str());
  return rv;
}

int VortexPort::writeData(ByteStream &stream)
{
  debug_send("%u %x > Writing buf: %u\n", g_counter++, GetCurrentThreadId(), stream.rawSize());
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
  if (!m_serialPort.writeData(buf.data(), buf.size())) {
    printf("BIG ERROR ~~~~~~~~~~~~~\n");
    return 0;
  }
#ifdef DEBUG_SENDING
  debug_send("%u %x >> Written buf: %u\n", g_counter++, GetCurrentThreadId(), buf.size());
  debug_send("\t");
  for (uint32_t i = 0; i < buf.size(); ++i) {
    debug_send("%02x ", buf.data()[i]);
    if ((i + 1) % 32 == 0) {
      debug_send("\n\t");
    }
  }
  debug_send("\n");
#endif
  return buf.size();
}

bool VortexPort::expectData(const std::string &data)
{
  debug_send("%u %x << Expecting data: %s\n", g_counter++, GetCurrentThreadId(), data.c_str());
  ByteStream stream;
  readInLoop(stream);
  if (stream.size() < data.size()) {
    return false;
  }
  debug_send("%u %x < Got expected data: %s\n", g_counter++, GetCurrentThreadId(), stream.data());
  if (data != (char *)stream.data()) {
    return false;
  }
  return true;
}

bool VortexPort::readInLoop(ByteStream &outStream, uint32_t timeoutMs)
{
  outStream.clear();
  uint32_t start = GetTickCount();
  debug_send("%u %x < Reading in loop\n", g_counter++, GetCurrentThreadId());
  while ((start + timeoutMs) >= GetTickCount()) {
    if (!readData(outStream)) {
      // error?
      continue;
    }
    if (!outStream.size()) {
      continue;
    }
    debug_send("%u %x << Read in loop: %s\n", g_counter++, GetCurrentThreadId(), outStream.data());
    return true;
  }
  debug_send("%u %x << Reading in loop failed\n", g_counter++, GetCurrentThreadId());
  return false;
}

bool VortexPort::parseHandshake(const ByteStream &handshake)
{
  string handshakeStr = (char *)handshake.data();
  debug_send("%u %x = Parsing handshake: [%s]\n", g_counter++, GetCurrentThreadId(), handshakeStr.c_str());
  // if there is a goodbye message then the gloveset just left the editor
  // menu and we cannot send it messages anymore
  if (handshakeStr.find(EDITOR_VERB_GOODBYE) == (handshakeStr.size() - (sizeof(EDITOR_VERB_GOODBYE) - 1))) {
    setActive(false);
    g_pEditor->triggerRefresh();
    // if still connected, return to listening
    if (isConnected()) {
      listen();
    }
    debug_send("%u %x == Parsed handshake: Goodbye\n", g_counter++, GetCurrentThreadId());
    return false;
  }
  // TODO: Parse the device info out of handshake
  debug_send("%u %x == Parsed handshake: Good\n", g_counter++, GetCurrentThreadId());
  // check the handshake for valid datastart  // looks good
  return true;
}

bool VortexPort::readByteStream(ByteStream &outModes)
{
  uint32_t size = 0;
  // first check how much is in the serial port
  int32_t amt = 0;
  debug_send("%u %x < Reading modes\n", g_counter++, GetCurrentThreadId());
  // wait till amount available is enough
  while (m_serialPort.bytesAvailable() < sizeof(size));
  debug_send("%u %x << Reading modes avail %u\n", g_counter++, GetCurrentThreadId(), m_serialPort.bytesAvailable());
  // read the size out of the serial port
  m_serialPort.readData((void *)&size, sizeof(size));
  if (!size || size > 4096) {
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
  debug_send("%u %x << Read modes %u\n", g_counter++, GetCurrentThreadId(), amtRead);
  return true;
}

DWORD __stdcall VortexPort::begin(void *ptr)
{
  VortexPort *port = (VortexPort *)ptr;
  if (!port) {
    return 0;
  }
  ByteStream handshake;
  while (!port->m_portActive) {
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
    port->m_portActive = true;
  }
  // send a message to trigger a UI refresh
  g_pEditor->triggerRefresh();
  // cleanup this thread this function is running in
  CloseHandle(port->m_hThread);
  port->m_hThread = nullptr;
  return 0;
}
