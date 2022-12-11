#pragma once

#include <Windows.h>
#include "VortexConfig.h"

// ===================================================================
//  Version Configurations

// The engine major version indicates the state of the save file,
// if any changes to the save format occur then the major version
// must increment so that the savefiles will not be loaded
#define EDITOR_VERSION_MAJOR  1

// A minor version simply indicates a bugfix or minor change that
// will not effect the save files produces by the engine. This means
// a savefile produced by 1.1 should be loadable by an engine on 1.2
// and vice versa. But an engine on 2.0 cannot share savefiles with
// either of the engines on version 1.1 or 1.2
#define EDITOR_VERSION_MINOR  0

// produces a number like 1.0
#define EDITOR_VERSION_NUMBER EDITOR_VERSION_MAJOR.EDITOR_VERSION_MINOR

// produces a string like "1.0"
#define EDITOR_VERSION EXPAND_AND_QUOTE(EDITOR_VERSION_NUMBER)

// Title of the editor window
#define EDITOR_TITLE      "Vortex Editor v" EDITOR_VERSION " (with Vortex Engine v" VORTEX_VERSION ")"
// Default width of the editor window
#define EDITOR_WIDTH      840
// Default height of the editor window
#define EDITOR_HEIGHT     680
// Background color of editor
#define BACK_COL          RGB(40, 40, 40)
