#include "VortexColorPicker.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "resource.h"

#include "Serial/Compression.h"

#define FIELD_EDIT_ID       55001

#define SATVAL_BOX_ID       56001
#define HUE_SLIDER_ID       56002
#define RED_SLIDER_ID       56003
#define GREEN_SLIDER_ID     56004
#define BLUE_SLIDER_ID      56005
#define PREVIEW_ID          56006

#define COLOR_HISTORY_ID    57001
#define SAVE_COLOR_ID       58001

using namespace std;

VortexColorPicker::VortexColorPicker() :
  m_isOpen(false),
  m_mutex(nullptr),
  m_loadThread(nullptr),
  m_svBitmaps(),
  m_hueBitmap(nullptr),
  m_redBitmap(nullptr),
  m_greenBitmap(nullptr),
  m_blueBitmap(nullptr),
  m_hIcon(nullptr),
  m_xPos(0),
  m_yPos(0),
  m_lastRefresh(0),
  m_lastCol(0),
  m_curHSV(0, 255, 255),
  m_curRGB(255, 0, 0),
  m_colorPickerWindow(),
  m_satValBox(),
  m_hueSlider(),
  m_colorPreview(),
  m_hueTextbox(),
  m_satTextbox(),
  m_valTextbox()
{
  m_mutex = CreateMutex(NULL, false, NULL);
}

VortexColorPicker::~VortexColorPicker()
{
  DestroyIcon(m_hIcon);
}

void VortexColorPicker::load()
{
  // generate the backgrounds for the sv box
  m_loadThread = CreateThread(NULL, 0, loadThread, this, 0, NULL);
}

DWORD __stdcall VortexColorPicker::loadThread(void *arg)
{
  VortexColorPicker *colorPicker = (VortexColorPicker *)arg;
  // grab the loading mutex
  WaitForSingleObject(colorPicker->m_mutex, INFINITE);

  // load the backgrounds
  colorPicker->genSVBackgrounds();
  // load the hue bitmap
  colorPicker->m_hueBitmap = colorPicker->genHueBackground(24, 256);

  colorPicker->m_redBitmap = colorPicker->genRGBBackground(24, 256, 1, 0, 0);
  colorPicker->m_greenBitmap = colorPicker->genRGBBackground(24, 256, 0, 1, 0);
  colorPicker->m_blueBitmap = colorPicker->genRGBBackground(24, 256, 0, 0, 1);

  // done loading
  CloseHandle(colorPicker->m_loadThread);
  colorPicker->m_loadThread = nullptr;
  ReleaseMutex(colorPicker->m_mutex);
  return 0;
}

bool VortexColorPicker::loaded()
{
  if (!m_mutex) {
    return true;
  }
  if (m_mutex && WaitForSingleObject(m_mutex, 0) != WAIT_OBJECT_0) {
    return false;
  }
  CloseHandle(m_mutex);
  m_mutex = nullptr;

  m_satValBox.setBackground(m_svBitmaps[0]);
  m_satValBox.redraw();

  m_hueSlider.setBackground(m_hueBitmap);
  m_hueSlider.redraw();

  m_redSlider.setBackground(m_redBitmap);
  m_redSlider.redraw();

  m_greenSlider.setBackground(m_greenBitmap);
  m_greenSlider.redraw();

  m_blueSlider.setBackground(m_blueBitmap);
  m_blueSlider.redraw();
  
  return true;
}

// initialize the color picker
bool VortexColorPicker::init(HINSTANCE hInst)
{
  // trigger the background loader
  load();

  // the color picker
  m_colorPickerWindow.init(hInst, "Vortex Color Picker", BACK_COL, 420, 420, this);
  m_colorPickerWindow.setVisible(false);
  m_colorPickerWindow.setCloseCallback(hideGUICallback);

  // the sat/val box
  m_satValBox.init(hInst, m_colorPickerWindow, "Saturation and Value", BACK_COL, 256, 256, 9, 10, SATVAL_BOX_ID, selectSVCallback);
  m_satValBox.setSelection(m_curHSV.sat, 255 - m_curHSV.val);
  m_satValBox.setVisible(true);
  m_satValBox.setEnabled(true);

  // the hue slider
  m_hueSlider.init(hInst, m_colorPickerWindow, "Hue", BACK_COL, 24, 256, 273, 10, HUE_SLIDER_ID, selectHCallback);
  m_hueSlider.setSelection(0, m_curHSV.hue);
  m_hueSlider.setDrawCircle(false);
  m_hueSlider.setDrawVLine(false);

  // the rgb sliders
  m_redSlider.init(hInst, m_colorPickerWindow, "Red", BACK_COL, 24, 256, 305, 10, RED_SLIDER_ID, selectRCallback);
  m_redSlider.setSelection(0, 0);
  m_redSlider.setDrawCircle(false);
  m_redSlider.setDrawVLine(false);

  m_greenSlider.init(hInst, m_colorPickerWindow, "Green", BACK_COL, 24, 256, 337, 10, GREEN_SLIDER_ID, selectGCallback);
  m_greenSlider.setSelection(0, 255);
  m_greenSlider.setDrawCircle(false);
  m_greenSlider.setDrawVLine(false);

  m_blueSlider.init(hInst, m_colorPickerWindow, "Blue", BACK_COL, 24, 256, 369, 10, BLUE_SLIDER_ID, selectBCallback);
  m_blueSlider.setSelection(0, 255);
  m_blueSlider.setDrawCircle(false);
  m_blueSlider.setDrawVLine(false);

  // preview box for current color
  m_colorPreview.init(hInst, m_colorPickerWindow, "", BACK_COL, 122, 96, 273, 274, PREVIEW_ID, nullptr);
  m_colorPreview.setActive(true);
  m_colorPreview.setColor(0xFF0000);
  m_colorPreview.setSelectable(false);

  // color picker history
  for (uint32_t i = 0; i < sizeof(m_colorHistory) / sizeof(m_colorHistory[0]); ++i) {
    m_colorHistory[i].init(hInst, m_colorPickerWindow, "", BACK_COL, 18, 18, 9 + (24 * i), 274, COLOR_HISTORY_ID + i, historyCallback);
    m_colorHistory[i].setActive(true);
    m_colorHistory[i].setColor(0x000000);
    m_colorHistory[i].setSelectable(false);
  }

  // the hsv labels
  m_hueLabel.init(hInst, m_colorPickerWindow, "Hue:", BACK_COL, 32, 20, 131, 275, 0, nullptr);
  m_satLabel.init(hInst, m_colorPickerWindow, "Sat:", BACK_COL, 32, 20, 136, 301, 0, nullptr);
  m_valLabel.init(hInst, m_colorPickerWindow, "Val:", BACK_COL, 32, 20, 137, 325, 0, nullptr);
  m_hueLabel.setBackColor(BACK_COL);

  // hsv text boxes
  m_hueTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.hue).c_str(), BACK_COL, 32, 20, 164, 274, FIELD_EDIT_ID + 0, fieldEditCallback);
  m_satTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.sat).c_str(), BACK_COL, 32, 20, 164, 300, FIELD_EDIT_ID + 1, fieldEditCallback);
  m_valTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.val).c_str(), BACK_COL, 32, 20, 164, 324, FIELD_EDIT_ID + 2, fieldEditCallback);
  m_hueTextbox.setEnabled(true);
  m_satTextbox.setEnabled(true);
  m_valTextbox.setEnabled(true);

  // the rgb labels
  m_redLabel.init(hInst, m_colorPickerWindow, "Red:", BACK_COL, 32, 20, 202, 275, 0, nullptr);
  m_grnLabel.init(hInst, m_colorPickerWindow, "Grn:", BACK_COL, 32, 20, 205, 301, 0, nullptr);
  m_bluLabel.init(hInst, m_colorPickerWindow, "Blu:", BACK_COL, 32, 20, 206, 325, 0, nullptr);
  
  // rgb text boxes
  m_redTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.hue).c_str(), BACK_COL, 32, 20, 235, 274, FIELD_EDIT_ID + 3, fieldEditCallback);
  m_grnTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.sat).c_str(), BACK_COL, 32, 20, 235, 300, FIELD_EDIT_ID + 4, fieldEditCallback);
  m_bluTextbox.init(hInst, m_colorPickerWindow, to_string(m_curHSV.val).c_str(), BACK_COL, 32, 20, 235, 324, FIELD_EDIT_ID + 5, fieldEditCallback);
  m_redTextbox.setEnabled(true);
  m_grnTextbox.setEnabled(true);
  m_bluTextbox.setEnabled(true);

  // hex label
  m_hexLabel.init(hInst, m_colorPickerWindow, "Hex:", BACK_COL, 32, 20, 131, 352, 0, nullptr);

  // hex text box
  m_hexTextbox.init(hInst, m_colorPickerWindow, "#FF0000", BACK_COL, 103, 20, 164, 350, FIELD_EDIT_ID + 6, fieldEditCallback);
  m_hexTextbox.setEnabled(true);

  m_savedColorsBox.init(hInst, m_colorPickerWindow, "", BACK_COL, 114, 70, 9, 300, 0, nullptr);
  m_savedColorsBox.setActive(true);
  m_savedColorsBox.setEnabled(false);
  m_savedColorsBox.setSelectable(false);
  m_savedColorsBox.setColor(0x1D1D1D);

  for (uint32_t i = 0; i < 8; ++i) {
    m_savedColors[i].init(hInst, m_colorPickerWindow, "", BACK_COL, 19, 19, 17 + (26 * (i % 4)), i < 4 ? 319 : 344, SAVE_COLOR_ID + i, saveColorCallback);
    m_savedColors[i].setSelectable(true);
    m_savedColors[i].setActive(true);
    m_savedColors[i].setColor(0x000000);
  }

  m_customColorsLabel.init(hInst, m_colorPickerWindow, "Saved Colors", 0x1D1D1D, 75, 15, 17, 302, 0, nullptr);

  HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 
    FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
  SendMessage(m_customColorsLabel.hwnd(), WM_SETFONT, WPARAM(hFont), TRUE);

  // apply the icon
  m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
  SendMessage(m_colorPickerWindow.hwnd(), WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);

  return true;
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
      //RGBColor rgbCol = hsv_to_rgb_generic(HSVColor(hue, x, 255 - y));
      RGBColor rgbCol = HSVColor(hue, x, 255 - y);
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

void VortexColorPicker::selectSV(VSelectBox::SelectEvent sevent, uint32_t s, uint32_t v)
{
  // value is inverted idk
  selectSV(s, 255 - v);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  pickCol();
}

void VortexColorPicker::selectH(VSelectBox::SelectEvent sevent, uint32_t h)
{
  selectH(h);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  pickCol();
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
  case 3: // red
    selectR(field->getValue());
    break;
  case 4: // grn
    selectG(field->getValue());
    break;
  case 5: // blu
    selectB(field->getValue());
    break;
  case 6: // hex
    setColor(RGBColor(strtoul(field->getText().c_str() + 1, NULL, 16)));
    break;
  default: // unknown
    // ???
    return;
  }
  uint32_t rawCol = m_curRGB.raw();
  g_pEditor->updateSelectedColors(rawCol);
  pushHistory(rawCol);
  for (uint32_t i = 0; i < 8; ++i) {
    if (m_savedColors[i].isSelected()) {
      m_savedColors[i].setColor(rawCol);
      m_savedColors[i].redraw();
      break;
    }
  }
  refreshColor();
  //demoCurMode();
}

void VortexColorPicker::selectR(VSelectBox::SelectEvent sevent, uint32_t r)
{
  selectR(255 - r);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  pickCol();
}

void VortexColorPicker::selectG(VSelectBox::SelectEvent sevent, uint32_t g)
{
  selectG(255 - g);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  pickCol();
}

void VortexColorPicker::selectB(VSelectBox::SelectEvent sevent, uint32_t b)
{
  selectB(255 - b);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  pickCol();
}

void VortexColorPicker::pickCol()
{
  if (!g_pEditor) {
    return;
  }
  // update all textboxes and do not notify of the change
  m_hueTextbox.setText(to_string(m_curHSV.hue).c_str(), false);
  m_satTextbox.setText(to_string(m_curHSV.sat).c_str(), false);
  m_valTextbox.setText(to_string(m_curHSV.val).c_str(), false);
  m_redTextbox.setText(to_string(m_curRGB.red).c_str(), false);
  m_grnTextbox.setText(to_string(m_curRGB.green).c_str(), false);
  m_bluTextbox.setText(to_string(m_curRGB.blue).c_str(), false);
  uint32_t rawCol = m_curRGB.raw();
  if (rawCol) {
    m_hexTextbox.setText(m_colorPreview.getColorName().c_str(), false);
  } else {
    // must set #000000 because colorSelect will return 'blank'
    m_hexTextbox.setText("#000000", false);
  }
  // redraw all of the textboxes
  m_hueTextbox.redraw();
  m_satTextbox.redraw();
  m_valTextbox.redraw();
  m_redTextbox.redraw();
  m_grnTextbox.redraw();
  m_bluTextbox.redraw();
  m_hexTextbox.redraw();
  g_pEditor->updateSelectedColors(rawCol);
  pushHistory(rawCol);
  for (uint32_t i = 0; i < 8; ++i) {
    if (m_savedColors[i].isSelected()) {
      m_savedColors[i].setColor(rawCol);
      m_savedColors[i].redraw();
      break;
    }
  }
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
    //RGBColor rgbCol = hsv_to_rgb_generic(HSVColor(y, 255, 255));
    RGBColor rgbCol = HSVColor(y, 255, 255);
    for (uint32_t x = 0; x < width; ++x) {
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

HBITMAP VortexColorPicker::genRGBBackground(uint32_t width, uint32_t height, int rmult, int gmult, int bmult)
{
  COLORREF *cols = new COLORREF[width * height];
  if (!cols) {
    return nullptr;
  }
  // the real x and y are the internal coords inside the border where as
  // m_width and m_height contain the border size in them
  for (uint32_t y = 0; y < height; ++y) {
    RGBColor rgbCol(rmult * (height - y), gmult * (height - y), bmult * (height - y));
    for (uint32_t x = 0; x < width; ++x) {
      cols[(y * width) + x] = rgbCol.raw();
    }
  }
  HBITMAP bitmap = CreateBitmap(width, height, 1, 32, cols);
  delete[] cols;
  return bitmap;
}

void VortexColorPicker::run()
{
}

void VortexColorPicker::show()
{
  if (m_isOpen) {
    return;
  }
  // wait till loaded...
  while (!loaded());
  m_colorPickerWindow.setVisible(true);
  m_colorPickerWindow.setEnabled(true);
  m_isOpen = true;
}

void VortexColorPicker::hide()
{
  if (!m_isOpen) {
    return;
  }
  if (m_colorPickerWindow.isVisible()) {
    m_colorPickerWindow.setVisible(false);
  }
  if (m_colorPickerWindow.isEnabled()) {
    m_colorPickerWindow.setEnabled(false);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    g_pEditor->m_colorSelects[i].setSelected(false);
    g_pEditor->m_colorSelects[i].redraw();
    m_savedColors[i].setSelected(false);
  }
  m_isOpen = false;
}

// push a color into the history
void VortexColorPicker::pushHistory(uint32_t rawCol)
{
  uint32_t numHistory = sizeof(m_colorHistory) / sizeof(m_colorHistory[0]);
  for (uint32_t i = 0; i < (numHistory - 1); ++i) {
    m_colorHistory[i].setColor(m_colorHistory[i + 1].getColor());
  }
  m_colorHistory[numHistory - 1].setColor(rawCol);
}

void VortexColorPicker::recallHistory(VColorSelect *history)
{
  m_curRGB = history->getColor();
  m_curHSV = m_curRGB;
  pickCol();
  refreshColor();
}

void VortexColorPicker::saveColor(VColorSelect *saveCol, VColorSelect::SelectEvent sevent)
{
  switch (sevent) {
  case VColorSelect::SelectEvent::SELECT_LEFT_CLICK:
  case VColorSelect::SelectEvent::SELECT_CTRL_LEFT_CLICK:
  case VColorSelect::SelectEvent::SELECT_SHIFT_LEFT_CLICK:
    if (!saveCol->isSelected()) {
      break;
    }
    // go over all the saved colors and unselected the others
    for (uint32_t i = 0; i < 8; ++i) {
      if (m_savedColors + i == saveCol) {
        continue;
      }
      if (m_savedColors[i].isSelected()) {
        m_savedColors[i].setSelected(false);
        m_savedColors[i].redraw();
      }
    }
    // if the slot has a color saved then pull it into the color picker
    if (saveCol->getColor()) {
      setColor(RGBColor(saveCol->getColor()));
      pickCol();
      refreshColor();
    }
    break;
  case VColorSelect::SelectEvent::SELECT_RIGHT_CLICK:
    // prevent the saved colors from going inactive (red border)
    if (!saveCol->isActive()) {
      saveCol->setActive(true);
    }
    break;
  }
  saveCol->redraw();
}

void VortexColorPicker::selectS(uint32_t sat)
{
  m_curHSV.sat = sat;
  m_curRGB = m_curHSV;
  refreshColor();
}

void VortexColorPicker::selectV(uint32_t val)
{
  m_curHSV.val = val;
  m_curRGB = m_curHSV;
  refreshColor();
}

void VortexColorPicker::selectSV(uint32_t sat, uint32_t ival)
{
  m_curHSV.sat = sat;
  m_curHSV.val = ival;
  m_curRGB = m_curHSV;
  refreshColor();
}

void VortexColorPicker::selectH(uint32_t hue)
{
  m_curHSV.hue = hue;
  m_curRGB = m_curHSV;
  refreshColor();
}

void VortexColorPicker::selectR(uint32_t r)
{
  m_curRGB.red = r;
  m_curHSV = m_curRGB;
  refreshColor();
}

void VortexColorPicker::selectG(uint32_t g)
{
  m_curRGB.green = g;
  m_curHSV = m_curRGB;
  refreshColor();
}

void VortexColorPicker::selectB(uint32_t b)
{
  m_curRGB.blue = b;
  m_curHSV = m_curRGB;
  refreshColor();
}

void VortexColorPicker::refreshColor()
{
  m_redSlider.setSelection(0, 255 - m_curRGB.red);
  m_greenSlider.setSelection(0, 255 - m_curRGB.green);
  m_blueSlider.setSelection(0, 255 - m_curRGB.blue);
  m_hueSlider.setSelection(0, m_curHSV.hue);
  m_satValBox.setSelection(m_curHSV.sat, 255 - m_curHSV.val);
  m_satValBox.setBackground(m_svBitmaps[m_curHSV.hue]);
  uint64_t now = GetCurrentTime();
  uint32_t rawCol = m_curRGB.raw();
  m_colorPreview.setColor(rawCol);
  if (m_lastCol != rawCol && now > m_lastRefresh) {
    g_pEditor->demoColor(rawCol);
    // if we want to carry through the updates to the colorset call this:
    //g_pEditor->updateSelectedColors(rawCol);
    // Could actually just update the mode here... However the gloveset is
    // programmed to reset the mode from the beginning when you change it so
    // you won't get a streaming coloret change like you'd expect
    //g_pEditor->demoCurMode();
    m_lastRefresh = now;
    m_lastCol = rawCol;
  }
  m_redSlider.redraw();
  m_greenSlider.redraw();
  m_blueSlider.redraw();
  m_satValBox.redraw();
  m_hueSlider.redraw();
  m_colorPreview.redraw();
}
