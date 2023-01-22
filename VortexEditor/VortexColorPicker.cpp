#include "VortexColorPicker.h"
#include "EditorConfig.h"

#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

VortexColorPicker::VortexColorPicker() :
  m_svBitmaps(),
  m_hueBitmap(nullptr),
  m_hIcon(nullptr),
  m_xPos(0),
  m_yPos(0),
  m_colorPickerWindow(),
  m_satValBox()
{
}

VortexColorPicker::~VortexColorPicker()
{
  DestroyIcon(m_hIcon);
}

// set a background via callback to convert x/y to rgb
void VortexColorPicker::genSVBackgrounds()
{
  m_svBitmaps[0] = genSVBackground(0);
  for (uint32_t i = 1; i < sizeof(m_svBitmaps) / sizeof(m_svBitmaps[0]); ++i) {
    m_svBitmaps[i] = m_svBitmaps[0];
  }
}

HBITMAP VortexColorPicker::genSVBackground(uint32_t hue)
{
  HDC hDC = GetDC(m_satValBox.hwnd());
  HDC memDC = CreateCompatibleDC(hDC);
  uint32_t width = 256;
  uint32_t height = 256;
  HBITMAP bitmap = CreateCompatibleBitmap(hDC, width, height);
  HGDIOBJ oldObj = SelectObject(memDC, bitmap);
printf("Generating hue %u\n", hue);
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      RGBColor rgbCol = hsv_to_rgb_rainbow(HSVColor(hue, x, 255 - y));
      COLORREF col = (rgbCol.blue << 16) | (rgbCol.green << 8) | (rgbCol.red);
      SetPixel(memDC, x, y, col);
    }
  }
  SelectObject(memDC, oldObj);
  DeleteDC(memDC);
  return bitmap;
}

HBITMAP VortexColorPicker::genHueBackground(uint32_t width, uint32_t height)
{
  HDC hDC = GetDC(m_satValBox.hwnd());
  HDC memDC = CreateCompatibleDC(hDC);
  HBITMAP bitmap = CreateCompatibleBitmap(hDC, width, height);
  HGDIOBJ oldObj = SelectObject(memDC, bitmap);
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    RGBColor rgbCol = hsv_to_rgb_rainbow(HSVColor(y, 255, 255));
    COLORREF col = (rgbCol.blue << 16) | (rgbCol.green << 8) | (rgbCol.red);
    for (uint32_t x = 0; x < width; ++x) {
      SetPixel(memDC, x, y, col);
    }
  }
  SelectObject(memDC, oldObj);
  DeleteDC(memDC);
  return bitmap;
}

// initialize the test framework
bool VortexColorPicker::init(HINSTANCE hInst)
{
  // the color picker
  m_colorPickerWindow.init(hInst, "Vortex Color Picker", BACK_COL, 420, 420, this);
  m_colorPickerWindow.setVisible(true);

  // generate the backgrounds for the sv box
  genSVBackgrounds();
  HBITMAP hueBack = genHueBackground(24, 256);

  // the color ring
  m_satValBox.init(hInst, m_colorPickerWindow, "Saturation and Value", BACK_COL, 256, 256, 10, 10, 0, selectSVCallback);
  m_satValBox.setVisible(true);
  m_satValBox.setEnabled(true);

  // the val slider
  m_hueSlider.init(hInst, m_colorPickerWindow, "Hue", BACK_COL, 24, 256, 272, 10, 0, selectHCallback);
  m_hueSlider.setBackground(hueBack);
  m_hueSlider.setDrawCircle(false);
  m_hueSlider.setDrawVLine(false);

  m_colorPreview.init(hInst, m_colorPickerWindow, "Preview", BACK_COL, 32, 32, 10, 280, 0, nullptr);
  m_colorPreview.setActive(true);
  m_colorPreview.setColor(0xFF0000);

  m_hueTextbox.init(hInst, m_colorPickerWindow, "Hue", BACK_COL, 50, 18, 300, 10, 0, nullptr);
  m_satTextbox.init(hInst, m_colorPickerWindow, "Sat", BACK_COL, 50, 18, 300, 50, 0, nullptr);
  m_valTextbox.init(hInst, m_colorPickerWindow, "Val", BACK_COL, 50, 18, 300, 90, 0, nullptr);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_colorPickerWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
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

// redraw all the components
void VortexColorPicker::triggerRefresh()
{
  m_satValBox.redraw();
  m_hueSlider.redraw();
}

void VortexColorPicker::selectSV(uint32_t sat, uint32_t ival)
{
  m_curColor.sat = sat;
  // value is inverted idk
  m_curColor.val = 255 - ival;
  m_colorPreview.setColor(RGBColor(m_curColor).raw());
  triggerRefresh();
}

void VortexColorPicker::selectH(uint32_t hue)
{
  m_curColor.hue = hue;
  m_satValBox.setBackground(m_svBitmaps[hue]);
  m_colorPreview.setColor(RGBColor(m_curColor).raw());
  triggerRefresh();
}
