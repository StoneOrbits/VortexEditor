#pragma once

#include <windows.h>
#include <string>

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

  // Read data in a buffer, if nbChar is greater than the
  // maximum number of bytes available, it will return only the
  // bytes available. The function return -1 when nothing could
  // be read, the number of bytes actually read.
  int ReadData(void *buffer, unsigned int nbChar);

  // Writes data from a buffer through the Serial connection
  // return true on success.
  bool WriteData(const void *buffer, unsigned int nbChar);

  // Check if we are actually connected
  bool IsConnected() const { return m_connected; }

  std::string portString() const { return m_port; }

private:
  std::string m_port;
  HANDLE m_hSerial;
  bool m_connected;
  // whether serial or pipe
  bool m_isSerial;
  COMSTAT m_status;
  DWORD m_errors;
};
