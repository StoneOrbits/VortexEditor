#include "ArduinoSerial.h"

#include "Serial/ByteStream.h"

#define ARDUINO_WAIT_TIME 2000

using namespace std;

ArduinoSerial::ArduinoSerial() :
  m_port(),
  m_portNum(0),
  m_hFile(nullptr),
  m_connected(false),
  m_isSerial(false),
  m_status(),
  m_errors(0)
{
}

ArduinoSerial::ArduinoSerial(const string &portName) :
  ArduinoSerial()
{
  connect(portName);
}

ArduinoSerial::ArduinoSerial(ArduinoSerial &&other) noexcept :
  ArduinoSerial()
{
  *this = move(other);
}

ArduinoSerial::~ArduinoSerial()
{
  disconnect();
}

void ArduinoSerial::operator=(ArduinoSerial &&other) noexcept
{
  m_port = other.m_port;
  m_portNum = other.m_portNum;
  m_hFile = other.m_hFile;
  m_connected = other.m_connected;
  m_status = other.m_status;
  m_errors = other.m_errors;
  m_isSerial = other.m_isSerial;

  other.m_port.clear();
  other.m_portNum = 0;
  other.m_hFile = nullptr;
  other.m_connected = false;
  memset(&other.m_status, 0, sizeof(other.m_status));
  other.m_errors = 0;
  other.m_isSerial = false;
}

bool ArduinoSerial::connect(const string &portName)
{
  m_port = portName;
  // We're not yet connected
  m_connected = false;
  m_isSerial = true;

  if (portName.find("\\\\.\\pipe") != std::string::npos) {
    m_isSerial = false;
  }

  if (portName.find("\\\\.\\COM") != std::string::npos) {
    m_portNum = strtoul(portName.c_str() + 7, NULL, 10);
  }

  // Try to connect to the given port throuh CreateFile
  m_hFile = CreateFile(m_port.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
    NULL);

  // Check if the connection was successfull
  if (m_hFile == INVALID_HANDLE_VALUE) {
    // If not success full display an Error
    int err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND) {
      printf("ERROR: %u\n", err);
    }
    return false;
  }
  // If connected we try to set the comm parameters
  DCB dcbSerialParams = { 0 };

  // if it's actually an arduino we must do this
  if (m_isSerial && !GetCommState(m_hFile, &dcbSerialParams)) {
    printf("ALERT: Could not get Serial Port parameters");
    return false;
  }
  // Define serial connection parameters for the arduino board
  dcbSerialParams.BaudRate = CBR_9600;
  dcbSerialParams.ByteSize = 8;
  dcbSerialParams.StopBits = ONESTOPBIT;
  dcbSerialParams.Parity = NOPARITY;
  // Setting the DTR to Control_Enable ensures that the Arduino is properly
  // reset upon establishing a connection
  dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
  // Set the parameters and check for their proper application
  if (m_isSerial && !SetCommState(m_hFile, &dcbSerialParams)) {
    printf("ALERT: Could not set Serial Port parameters");
    return false;
  }
  // If everything went fine we're connected
  m_connected = true;
  if (m_isSerial) {
    // Flush any remaining characters in the buffers
    PurgeComm(m_hFile, PURGE_RXCLEAR | PURGE_TXCLEAR);
    // We wait 2s as the arduino board will be reseting
    //Sleep(ARDUINO_WAIT_TIME);
  }
  return true;
}

void ArduinoSerial::disconnect()
{
  // We're no longer connected
  m_connected = false;
  if (m_hFile) {
    if (!m_isSerial) {
      DisconnectNamedPipe(m_hFile);
    } else {
      // Close the serial handler
      CloseHandle(m_hFile);
    }
    m_hFile = nullptr;
  }
}

// amount of data ready
int ArduinoSerial::bytesAvailable()
{
  if (!m_isSerial) {
    DWORD toRead = 0;
    PeekNamedPipe(m_hFile, 0, 0, 0, (LPDWORD)&toRead, 0);
    return toRead;
  }
  ClearCommError(m_hFile, &m_errors, &m_status);
  return m_status.cbInQue;
}

// wait till data is available
int ArduinoSerial::rawRead(void *buffer, uint32_t amount)
{
  DWORD bytesRead = 0;
  // Try to read the require number of chars, and return the number of read bytes on success
  if (!ReadFile(m_hFile, buffer, amount, &bytesRead, NULL)) {
    return 0;
  }
  return bytesRead;
}

int ArduinoSerial::readData(void *buffer, uint32_t nbChar)
{
  // Number of bytes we'll really ask to read
  uint32_t toRead = 0;
  // Number of bytes we'll have read
  DWORD bytesRead = 0;
  // Use the ClearCommError function to get status info on the Serial port
  ClearCommError(m_hFile, &m_errors, &m_status);
  if (m_isSerial) {
    // Check if there is something to read
    if (!m_status.cbInQue) {
      return 0;
    }
  } else {
    if (!m_isSerial && !PeekNamedPipe(m_hFile, 0, 0, 0, (LPDWORD)&m_status.cbInQue, 0)) {
      // Handle failure.
    }
  }
  // If there is we check if there is enough data to read the required number
  // of characters, if not we'll read only the available characters to prevent
  // locking of the application.
  if (m_status.cbInQue > nbChar) {
    toRead = nbChar;
  } else {
    toRead = m_status.cbInQue;
  }
  if (!buffer || !nbChar) {
    return m_status.cbInQue;
  }
  // Try to read the require number of chars, and return the number of read bytes on success
  if (!ReadFile(m_hFile, buffer, toRead, &bytesRead, NULL)) {
    int err = GetLastError();
    if (!m_isSerial && err == ERROR_BROKEN_PIPE) {
      disconnect();
    }
    // If nothing has been read, or that an error was detected return 0
    return 0;
  }
  return bytesRead;
}

bool ArduinoSerial::writeData(const void *buffer, uint32_t nbChar)
{
  DWORD bytesSend = 0;

  // write one byte at a time, otherwise we could fill up the serial buffer
  // with a single send of 64bytes and the arduino will never receive it because
  // a bug in the arduino code seems to make blocksizes of data just show up as 0
  //    https://github.com/arduino/ArduinoCore-avr/issues/112
  for (uint32_t i = 0; i < nbChar; ++i) {
    DWORD sent = 0;
    // Try to write the buffer on the Serial port
    if (!WriteFile(m_hFile, ((uint8_t *)buffer) + i, 1, &sent, 0)) {
      if (m_isSerial) {
        // In case it don't work get comm error and return false
        ClearCommError(m_hFile, &m_errors, &m_status);
      }
      return false;
    }
    bytesSend += sent;
  }
  if (bytesSend < nbChar) {
    MessageBox(NULL, "Failed to full send", "", 0);
    return false;
  }
  return true;
}

bool ArduinoSerial::isConnected() const
{
  return m_connected;
}
