#include "VortexColorPicker.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "EngineWrapper.h"

#include "resource.h"

#include "Serial/Compression.h"

#define FIELD_EDIT_ID       55001

using namespace std;

VortexColorPicker::VortexColorPicker() :
  m_svBitmaps(),
  m_hueBitmap(nullptr),
  m_hIcon(nullptr),
  m_xPos(0),
  m_yPos(0),
  m_curColor(0, 255, 255),
  m_colorPickerWindow(),
  m_satValBox(),
  m_hueSlider(),
  m_colorPreview(),
  m_hueTextbox(),
  m_satTextbox(),
  m_valTextbox()
{
}

VortexColorPicker::~VortexColorPicker()
{
  DestroyIcon(m_hIcon);
}

// set a background via callback to convert x/y to rgb
void VortexColorPicker::genSVBackgrounds()
{
  for (uint32_t i = 0; i < sizeof(m_svBitmaps) / sizeof(m_svBitmaps[0]); ++i) {
    m_svBitmaps[i] = genSVBackground(i);
  }
}

HBITMAP VortexColorPicker::genSVBackground(uint32_t hue)
{
  uint32_t width = 256;
  uint32_t height = 256;
  COLORREF *cols = new COLORREF[256 * 256];
  if (!cols) {
    return nullptr;
  }
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      RGBColor rgbCol = hsv_to_rgb_generic(HSVColor(hue, x, 255 - y));
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

void VortexColorPicker::selectSV(VSelectBox::SelectEvent sevent, uint32_t s, uint32_t v)
{
  selectSV(s, v);
  if (!g_pEditor->m_selectedColorSlot || sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  uint32_t rawCol = hsv_to_rgb_generic(m_curColor).raw();
  g_pEditor->updateSelectedColor(rawCol);
  return;
#if 0
  m_colorPreview.setColor(rawCol);
  g_pEditor->m_selectedColorSlot->setColor(rawCol);
  Colorset newSet;
  VEngine::getColorset((LedPos)pos, newSet);
  // if the color select was made inactive
  if (!colSelect->isActive()) {
    debug("Disabled color slot %u", colorIndex);
    newSet.removeColor(colorIndex);
  } else {
    debug("Updating color slot %u", colorIndex);
    newSet.set(colorIndex, colSelect->getColor()); // getRawColor?
  }
  vector<int> sels;
  m_ledsMultiListBox.getSelections(sels);
  if (!sels.size()) {
    // this should never happen
    return;
  }
  // set the colorset on all selected patterns
  for (uint32_t i = 0; i < sels.size(); ++i) {
    // only set the pattern on a single position
    VEngine::setColorset((LedPos)sels[i], newSet);
  }
  refreshModeList();
  // update the demo
  demoCurMode();
#endif
}

void VortexColorPicker::selectH(VSelectBox::SelectEvent sevent, uint32_t h)
{
  selectH(h);
  if (!g_pEditor->m_selectedColorSlot || sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  uint32_t rawCol = hsv_to_rgb_generic(m_curColor).raw();
  g_pEditor->updateSelectedColor(rawCol);
}

void VortexColorPicker::fieldEdit(VWindow *window)
{
  VTextBox *field = (VTextBox *)window;
  if (!window || !window->isEnabled()) {
    return;
  }
  uint32_t fieldIndex = (uint32_t)((uintptr_t)window->menu() - FIELD_EDIT_ID);
  switch (fieldIndex) {
  case 0: // hue
    selectH(field->getValue());
    break;
  case 1: // sat
    selectS(field->getValue());
    break;
  case 2: // val
    selectV(field->getValue());
    break;
  }
  //demoCurMode();
}

HBITMAP VortexColorPicker::genHueBackground(uint32_t width, uint32_t height)
{
  COLORREF *cols = new COLORREF[256 * 256];
  if (!cols) {
    return nullptr;
  }
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    RGBColor rgbCol = hsv_to_rgb_generic(HSVColor(y, 255, 255));
    for (uint32_t x = 0; x < width; ++x) {
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

HBITMAP VortexColorPicker::genRedBackground(uint32_t width, uint32_t height)
{
  COLORREF *cols = new COLORREF[256 * 256];
  if (!cols) {
    return nullptr;
  }
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    RGBColor rgbCol(y, 0, 0);
    for (uint32_t x = 0; x < width; ++x) {
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

// initialize the test framework
bool VortexColorPicker::init(HINSTANCE hInst)
{
  // the color picker
  m_colorPickerWindow.init(hInst, "Vortex Color Picker", BACK_COL, 420, 420, this);
  m_colorPickerWindow.setVisible(true);

  // generate the backgrounds for the sv box
  // TODO: Do this on a background worker thread?
  genSVBackgrounds();
  m_hueBitmap = genHueBackground(24, 256);
  HBITMAP m_redBitmap = genRedBackground(24, 256);

  // the sat/val box
  m_satValBox.init(hInst, m_colorPickerWindow, "Saturation and Value", BACK_COL, 256, 256, 10, 10, 0, selectSVCallback);
  m_satValBox.setBackground(m_svBitmaps[0]);
  m_satValBox.setSelection(m_curColor.sat, 255 - m_curColor.val);
  m_satValBox.setVisible(true);
  m_satValBox.setEnabled(true);

  // the hue slider
  m_hueSlider.init(hInst, m_colorPickerWindow, "Hue", BACK_COL, 24, 256, 272, 10, 0, selectHCallback);
  m_hueSlider.setBackground(m_hueBitmap);
  m_hueSlider.setSelection(0, m_curColor.hue);
  m_hueSlider.setDrawCircle(false);
  m_hueSlider.setDrawVLine(false);

  // the rgb sliders
  //m_redSlider.init(hInst, m_colorPickerWindow, "Red", BACK_COL, 24, 256, 306, 10, 0, selectRedCallback);
  //m_redSlider.setBackground(m_redBitmap);
  //m_redSlider.setSelection(0, m_curColor.hue);

  m_colorPreview.init(hInst, m_colorPickerWindow, "Preview", BACK_COL, 32, 32, 10, 280, 0, nullptr);
  m_colorPreview.setActive(true);
  m_colorPreview.setColor(0xFF0000);

  //m_hueTextbox.init(hInst, m_colorPickerWindow, to_string(m_curColor.hue).c_str(), BACK_COL, 50, 18, 306, 12, FIELD_EDIT_ID + 0, hueEditCallback);
  //m_satTextbox.init(hInst, m_colorPickerWindow, to_string(m_curColor.sat).c_str(), BACK_COL, 50, 18, 306, 42, FIELD_EDIT_ID + 1, satEditCallback);
  //m_valTextbox.init(hInst, m_colorPickerWindow, to_string(m_curColor.val).c_str(), BACK_COL, 50, 18, 306, 72, FIELD_EDIT_ID + 2, valEditCallback);

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

void VortexColorPicker::selectS(uint32_t sat)
{
  m_curColor.sat = sat;
  refreshColor();
}

void VortexColorPicker::selectV(uint32_t val)
{
  m_curColor.val = val;
  refreshColor();
}

void VortexColorPicker::selectSV(uint32_t sat, uint32_t ival)
{
  m_curColor.sat = sat;
  // value is inverted idk
  m_curColor.val = 255 - ival;
  refreshColor();
}

void VortexColorPicker::selectH(uint32_t hue)
{
  m_curColor.hue = hue;
  refreshColor();
}

void VortexColorPicker::refreshColor()
{
  m_satValBox.setBackground(m_svBitmaps[m_curColor.hue]);
  uint32_t rawCol = hsv_to_rgb_generic(m_curColor).raw();
  m_colorPreview.setColor(rawCol);
  g_pEditor->demoColor(rawCol);
  triggerRefresh();
}
