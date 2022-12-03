#include "FastLED.h"
#ifdef LINUX_FRAMEWORK
#include "TestFrameworkLinux.h"
#else
#include "VortexEditor.h"
#endif

// global instance
FastLEDClass FastLED;

// called when the user calls FastLED.addLeds
void FastLEDClass::init(CRGB *cl, int count)
{
  // TODO:
  //g_pTestFramework->installLeds(cl, count);
}

// called when user calls FastLED.setBrightness
void FastLEDClass::setBrightness(int brightness)
{
  // TODO:
  //g_pTestFramework->setBrightness(brightness);
}

// called when user calls FastLED.show
void FastLEDClass::show(uint32_t brightness)
{
  setBrightness(brightness);
  // TODO:
  //g_pTestFramework->show();
}

