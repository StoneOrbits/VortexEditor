#include "ArduinoSerial.h"

#define ARDUINO_WAIT_TIME 2000

using namespace std;

ArduinoSerial::ArduinoSerial() :
  m_port(),
  m_hSerial(nullptr),
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
  // Check if we are connected before trying to disconnect
  if (m_connected) {
    // We're no longer connected
    m_connected = false;
    // Close the serial handler
    CloseHandle(m_hSerial);
    m_hSerial = nullptr;
  }
}

void ArduinoSerial::operator=(ArduinoSerial &&other) noexcept
{
  m_port = other.m_port;
  m_hSerial = other.m_hSerial;
  m_connected = other.m_connected;
  m_status = other.m_status;
  m_errors = other.m_errors;
  m_isSerial = other.m_isSerial;

  other.m_port.clear();
  other.m_hSerial = nullptr;
  other.m_connected = false;
  other.m_isSerial = false;
  memset(&other.m_status, 0, sizeof(other.m_status));
  other.m_errors = 0;
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

  if (portName.substr(0, 3) == "COM") {
    m_portNum = strtoul(portName.c_str() + 3, NULL, 10);
  }

  // Try to connect to the given port throuh CreateFile
  m_hSerial = CreateFile(m_port.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
    NULL);

  // Check if the connection was successfull
  if (m_hSerial == INVALID_HANDLE_VALUE) {
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
  if (m_isSerial && !GetCommState(m_hSerial, &dcbSerialParams)) {
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
  if (m_isSerial && !SetCommState(m_hSerial, &dcbSerialParams)) {
    printf("ALERT: Could not set Serial Port parameters");
    return false;
  }
  // If everything went fine we're connected
  m_connected = true;
  if (m_isSerial) {
    // Flush any remaining characters in the buffers
    PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    // We wait 2s as the arduino board will be reseting
    //Sleep(ARDUINO_WAIT_TIME);
  }
  return true;
}

int ArduinoSerial::ReadData(void *buffer, unsigned int nbChar)
{
  // Number of bytes we'll have read
  DWORD bytesRead;
  // Number of bytes we'll really ask to read
  unsigned int toRead;

  // Use the ClearCommError function to get status info on the Serial port
  ClearCommError(m_hSerial, &m_errors, &m_status);

  // Check if there is something to read
  if (!m_isSerial || m_status.cbInQue > 0) {
    // If there is we check if there is enough data to read the required number
    // of characters, if not we'll read only the available characters to prevent
    // locking of the application.
    if (m_status.cbInQue > nbChar) {
      toRead = nbChar;
    } else {
      toRead = m_status.cbInQue;
    }

    if (!m_isSerial && !PeekNamedPipe(m_hSerial, 0, 0, 0, (LPDWORD)&toRead, 0)) {
      // Handle failure.
    }

    if (!buffer || !nbChar) {
      return m_isSerial ? m_status.cbInQue : toRead;
    }

    // Try to read the require number of chars, and return the number of read bytes on success
    if (ReadFile(m_hSerial, buffer, toRead, &bytesRead, NULL)) {
      return bytesRead;
    }
  }

  // If nothing has been read, or that an error was detected return 0
  return 0;
}

bool ArduinoSerial::WriteData(const void *buffer, unsigned int nbChar)
{
  DWORD bytesSend;

  // Try to write the buffer on the Serial port
  if (!WriteFile(m_hSerial, buffer, nbChar, &bytesSend, 0)) {
    if (m_isSerial) {
      // In case it don't work get comm error and return false
      ClearCommError(m_hSerial, &m_errors, &m_status);
    }
    return false;
  }
  if (bytesSend < nbChar) {
    MessageBox(NULL, "Failed to full send", "", 0);
    return false;
  }
  // FILE_FLAG_NO_BUFFERING is enabled
  //FlushFileBuffers(m_hSerial);
  return true;
}
