#include "EngineWrapper.h"

// VortexEngine includes
#include "VortexEngine.h"
#include "Serial/ByteStream.h"
#include "Patterns/Pattern.h"
#include "Colors/Colorset.h"
#include "EditorConfig.h"
#include "Modes/Modes.h"
#include "Modes/Mode.h"
#include "Modes/ModeBuilder.h"

// for random()
#include "Arduino.h"

using namespace std;

VEngine::VEngine()
{
}

void VEngine::init()
{
  // init the engine
  VortexEngine::init();
  // clear the modes
  Modes::clearModes();
}

void VEngine::cleanup()
{
}

bool VEngine::getModes(ByteStream &outStream)
{
  Modes::saveStorage();
  Modes::serialize(outStream);
  outStream.recalcCRC();
  return true;
}

bool VEngine::setModes(ByteStream &stream)
{
  if (!stream.checkCRC()) {
    //printf("BAD CRC !\n");
    return false;
  }
  // now unserialize the stream of data that was read
  if (!Modes::unserialize(stream)) {
    //printf("Unserialize failed\n");
    return false;
  }
  Modes::saveStorage();
  return true;

}

uint32_t VEngine::curMode()
{
  return Modes::curModeIndex();
}

uint32_t VEngine::numModes()
{
  return Modes::numModes();
}

bool VEngine::addNewMode()
{
  Colorset set;
  set.randomize();
    // create a random pattern ID from all patterns
  PatternID randomPattern;
  do {
    // continuously re-randomize the pattern so we don't get solids
    randomPattern = (PatternID)random(PATTERN_FIRST, PATTERN_COUNT);
  } while (randomPattern >= PATTERN_SOLID0 && randomPattern <= PATTERN_SOLID2);
  if (!Modes::addMode(randomPattern, &set)) {
    return false;
  }
  Modes::saveStorage();
  return true;
}

bool VEngine::setCurMode(uint32_t index)
{
  Modes::setCurMode(index);
  return true;
}

bool VEngine::nextMode()
{
  Modes::nextMode();
  return true;
}

bool VEngine::delCurMode()
{
  Modes::deleteCurMode();
  Modes::saveStorage();
  return true;
}

bool VEngine::setPattern(PatternID id)
{
  Mode *pMode = Modes::curMode();
  if (!pMode) {
    return false;
  }
  if (!pMode->setPattern(id)) {
    return false;
  }
  Modes::saveStorage();
  return true;
}

PatternID VEngine::getPatternID(LedPos pos)
{
  Mode *pMode = Modes::curMode();
  if (!pMode) {
    return PATTERN_NONE;
  }
  return pMode->getPatternID(pos);
}

string VEngine::getPatternName(LedPos pos)
{
  return patternToString(getPatternID(pos));
}

bool VEngine::setSinglePat(LedPos pos, PatternID id)
{
  Mode *pMode = Modes::curMode();
  if (!pMode) {
    return false;
  }
  if (!pMode->setSinglePat(pos, id)) {
    return false;
  }
  Modes::saveStorage();
  return true;
}

bool VEngine::getColorset(LedPos pos, Colorset &set)
{
  Mode *pMode = Modes::curMode();
  if (!pMode) {
    return false;
  }
  const Colorset *pSet = pMode->getColorset(pos);
  if (!pSet) {
    return false;
  }
  set = *pSet;
  return true;
}

bool VEngine::setColorset(LedPos pos, const Colorset &set)
{
  Mode *pMode = Modes::curMode();
  if (!pMode) {
    return false;
  }
  Pattern *pat = pMode->getPattern(pos);
  if (!pat) {
    return false;
  }
  pat->setColorset(&set);
  Modes::saveStorage();
  return true;
}

string VEngine::patternToString(PatternID id)
{
  if (id == PATTERN_NONE || id >= PATTERN_COUNT) {
    return "pattern_none";
  }
  static const char *patternNames[PATTERN_COUNT] = {
    "basic", "strobe", "hyperstrobe", "dops", "dopish", "ultradops", "strobie",
    "ribbon", "miniribbon", "tracer", "dashdops", "blinkie", "ghostcrush",
    "advanced", "blend", "complementary blend", "brackets", "solid0", "solid1",
    "solid2", "rabbit", "hueshift", "theater chase", "chaser", "zigzag",
    "zipfade", "tiptop", "drip", "dripmorph", "crossdops", "doublestrobe",
    "meteor", "sparkletrace", "vortexwipe", "warp", "warpworm", "snowball",
    "lighthouse", "pulsish", "fill", "bounce", "impact", "splitstrobie",
    "backstrobe", "flowers", "jest", "materia"
  };
  return patternNames[id];
}

string VEngine::ledToString(LedPos pos)
{
  if (pos >= LED_COUNT) {
    return "led_none";
  }
  static const char *ledNames[LED_COUNT] = {
    // tips       tops
    "pinkie tip", "pinkie top",
    "ring tip",   "ring top",
    "middle tip", "middle top",
    "index tip",  "index top",
    "thumb tip",  "thumb top",
  };
  return ledNames[pos];
}
