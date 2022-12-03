#include <Windows.h>

#include "VortexEditor.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  VortexEditor editor;
  editor.init(hInstance);
  editor.run();
  return 0;
}