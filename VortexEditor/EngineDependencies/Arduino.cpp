#include "Arduino.h"

#ifdef LINUX_FRAMEWORK
#include "TestFrameworkLinux.h"
#include <sys/socket.h>
#include <time.h>
#else
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include "VortexEditor.h"
#endif

#include <chrono>
#include <random>
#include <time.h>

SerialClass Serial;

#ifdef LINUX_FRAMEWORK
static uint64_t start;
#define SOCKET int
#else
static LARGE_INTEGER start;
static LARGE_INTEGER tps; //tps = ticks per second
#pragma comment (lib, "ws2_32.lib")
#define DEFAULT_PORT "33456"
SOCKET sock = -1;
SOCKET client_sock = -1;
bool is_server = false;
HANDLE hServerMutex = NULL;
bool is_ir_server() { return is_server; }
static bool receive_message(uint32_t &out_message);
static bool send_network_message(uint32_t message);
static bool accept_connection();
static DWORD __stdcall listen_connection(void *arg);
#endif

void init_arduino()
{
  QueryPerformanceFrequency(&tps);
  QueryPerformanceCounter(&start);
}

void delay(size_t amt)
{
  Sleep((DWORD)amt);
}

void delayMicroseconds(size_t us)
{
}

// used for seeding randomSeed()
unsigned long analogRead(uint32_t pin)
{
  return 0;
}

// used to read button input
unsigned long digitalRead(uint32_t pin)
{
  if (pin == 1) {
    // get button state
    // TODO:
    //if (g_pEditor->isButtonPressed()) {
    //  return LOW;
    //}
    return HIGH;
  }
  return HIGH;
}

void digitalWrite(uint32_t pin,  uint32_t val)
{
}

/// Convert seconds to milliseconds
#define SEC_TO_MS(sec) ((sec)*1000)
/// Convert seconds to microseconds
#define SEC_TO_US(sec) ((sec)*1000000)
/// Convert seconds to nanoseconds
#define SEC_TO_NS(sec) ((sec)*1000000000)

/// Convert nanoseconds to seconds
#define NS_TO_SEC(ns)   ((ns)/1000000000)
/// Convert nanoseconds to milliseconds
#define NS_TO_MS(ns)    ((ns)/1000000)
/// Convert nanoseconds to microseconds
#define NS_TO_US(ns)    ((ns)/1000)

unsigned long millis()
{
  return (unsigned long)GetTickCount();
}

unsigned long micros()
{
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  if (tps.QuadPart == 0) {
    return 0;
  }
  // yes, this will overflow, that's how arduino micros() works *shrug*
  return (unsigned long)((now.QuadPart - start.QuadPart) * 1000000 / tps.QuadPart);
}

unsigned long random(uint32_t low, uint32_t high)
{
  return low + (rand() % (high - low));
}


void randomSeed(uint32_t seed)
{
  srand((uint32_t)GetTickCount() ^ (uint32_t)GetCurrentProcessId());
}

void pinMode(uint32_t pin, uint32_t mode)
{
  // ???
}

void attachInterrupt(int interrupt, void (*func)(), int type)
{
}

void detachInterrupt(int interrupt)
{
}

int digitalPinToInterrupt(int pin)
{
  return 0;
}
