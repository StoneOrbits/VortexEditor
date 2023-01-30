#pragma once

#include <windows.h>
#include <string>

class ByteStream;

class ArduinoSerial
{
public:
  ArduinoSerial();
  // Initialize Serial communication with the given COM port
  ArduinoSerial(const std::string &portName);
  ArduinoSerial(ArduinoSerial &&other) noexcept;
  // Close the connection
  ~ArduinoSerial();

  // move assignment operator
  void operator=(ArduinoSerial &&other) noexcept;

  bool connect(const std::string &portName);
  void disconnect();

  // amount of data ready
  int bytesAvailable();

  // wait till data is available
  int rawRead(void *buffer, uint32_t amount);

  // Read data in a buffer, if nbChar is greater than the
  // maximum number of bytes available, it will return only the
  // bytes available. The function return -1 when nothing could
  // be read, the number of bytes actually read.
  int readData(void *buffer, uint32_t nbChar);

  // Writes data from a buffer through the Serial connection
  // return true on success.
  bool writeData(const uint8_t *buffer, uint32_t nbChar);

  // Check if we are actually connected
  bool isConnected() const;

  std::string portString() const { return m_port; }
  uint32_t portNumber() const { return m_portNum; }

private:
  std::string m_port;
  uint32_t m_portNum;
  HANDLE m_hFile;
  bool m_connected;
  // whether serial or pipe
  bool m_isSerial;
  COMSTAT m_status;
  DWORD m_errors;
};
