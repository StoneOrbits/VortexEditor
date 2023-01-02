#pragma once

#include "ArduinoSerial.h"

#include <string>

class VortexPort
{
public:
  VortexPort();
  VortexPort(const std::string &portName);
  VortexPort(VortexPort &&other) noexcept;
  ~VortexPort();
  void operator=(VortexPort &&other) noexcept;
  bool tryBegin();
  void listen();
  bool isConnected() const;
  bool isActive() const;
  ArduinoSerial &port();
  // set whether active or not
  void setActive(bool active);
  // amount of data ready
  int bytesAvailable();
  // read out any available data
  int readData(ByteStream &stream);
  // wait till data arrives then read it out
  int waitData(ByteStream &stream);
  // write a message to the port
  int writeData(const std::string &message);
  // write a buffer of binary data to the port
  int writeData(ByteStream &stream);
  // wait for some data
  bool expectData(const std::string &data);
  // read data in a loop
  void readInLoop(ByteStream &outStream);
  // helper to validate a handshake message
  bool parseHandshake(const ByteStream &handshakewindow);
  // read out the full list of modes
  bool readModes(ByteStream &outModes);
private:
  // the raw serial connection
  ArduinoSerial m_serialPort;
  // a handle to the thread that waits for the initial handshake
  HANDLE m_hThread;
  // whether the port is 'active' ie the handshake has been received
  bool m_portActive;
  // thread func to wait and begin a port connection
  static DWORD __stdcall begin(void *ptr);
};
