#include "VortexColorPicker.h"
#include "EditorConfig.h"

#include "resource.h"

VortexColorPicker::VortexColorPicker() :
  m_xPos(0),
  m_yPos(0),
  m_colorPickerWindow(),
  m_hueSatBox()
{
}

VortexColorPicker::~VortexColorPicker()
{
}

// initialize the test framework
bool VortexColorPicker::init(HINSTANCE hInst)
{
  // the color picker
  m_colorPickerWindow.init(hInst, "Vortex Color Picker", BACK_COL, 420, 420, nullptr);
  m_colorPickerWindow.setVisible(true);

  // the color ring
  m_hueSatBox.init(hInst, m_colorPickerWindow, "Hue and Saturation", BACK_COL, 259, 259, 10, 10, 0, nullptr);
  m_hueSatBox.setVisible(true);
  m_hueSatBox.setEnabled(true);
  m_hueSatBox.setActive(true);

  // the val slider
  m_valSlider.init(hInst, m_colorPickerWindow, "Brightness", BACK_COL, 24, 255, 272, 10, 0, nullptr);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_colorPickerWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
}

// run the test framework
void VortexColorPicker::run()
{
}

void VortexColorPicker::show()
{
  m_colorPickerWindow.setVisible(true);
  m_colorPickerWindow.setEnabled(true);
}

void VortexColorPicker::hide()
{
  m_colorPickerWindow.setVisible(false);
  m_colorPickerWindow.setEnabled(false);
}

// send the refresh message to the window
void VortexColorPicker::triggerRefresh()
{
}
