#include "VortexModeRandomizer.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

using namespace std;

VortexModeRandomizer::VortexModeRandomizer() :
  m_isOpen(false),
  m_hIcon(nullptr),
  m_modeRandomizerWindow()
{
}

VortexModeRandomizer::~VortexModeRandomizer()
{
  DestroyIcon(m_hIcon);
}

// initialize the color picker
bool VortexModeRandomizer::init(HINSTANCE hInst)
{
  // the color picker
  m_modeRandomizerWindow.init(hInst, "Vortex Mode Randomizer", BACK_COL, 690, 420, this);
  m_modeRandomizerWindow.setVisible(false);
  m_modeRandomizerWindow.setCloseCallback(hideGUICallback);
  m_modeRandomizerWindow.installLoseFocusCallback(loseFocusCallback);

  // create stuff

  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  //SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_modeRandomizerWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
}

void VortexModeRandomizer::run()
{
}

void VortexModeRandomizer::show()
{
  if (m_isOpen) {
    return;
  }
  m_modeRandomizerWindow.setVisible(true);
  m_modeRandomizerWindow.setEnabled(true);
  m_isOpen = true;
}

void VortexModeRandomizer::hide()
{
  if (!m_isOpen) {
    return;
  }
  if (m_modeRandomizerWindow.isVisible()) {
    m_modeRandomizerWindow.setVisible(false);
  }
  if (m_modeRandomizerWindow.isEnabled()) {
    m_modeRandomizerWindow.setEnabled(false);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    g_pEditor->m_colorSelects[i].setSelected(false);
    g_pEditor->m_colorSelects[i].redraw();
  }
  m_isOpen = false;
}

void VortexModeRandomizer::loseFocus()
{
}

