#include "VortexColorPicker.h"
#include "VortexEditor.h"
#include "EditorConfig.h"

#include "Colors/Colorset.h"
#include "Colors/Colortypes.h"

#include "EngineWrapper.h"

#include "resource.h"

#include "Serial/Compression.h"

#define FIELD_EDIT_ID       55001
#define SATVAL_BOX_ID       56001
#define HUE_SLIDER_ID       56002
#define RED_SLIDER_ID       56003
#define GREEN_SLIDER_ID     56004
#define BLUE_SLIDER_ID      56005

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
  load();

  // the color picker
  m_colorPickerWindow.init(hInst, "Vortex Color Picker", BACK_COL, 420, 420, this);
  m_colorPickerWindow.setVisible(false);
  m_colorPickerWindow.setCloseCallback(hideGUICallback);

  // trigger the background loader
  //HBITMAP m_redBitmap = genRedBackground(24, 256);

  // the sat/val box
  m_satValBox.init(hInst, m_colorPickerWindow, "Saturation and Value", BACK_COL, 256, 256, 10, 10, SATVAL_BOX_ID, selectSVCallback);
  m_satValBox.setSelection(m_curHSV.sat, 255 - m_curHSV.val);
  m_satValBox.setVisible(true);
  m_satValBox.setEnabled(true);

  // the hue slider
  m_hueSlider.init(hInst, m_colorPickerWindow, "Hue", BACK_COL, 24, 256, 274, 10, HUE_SLIDER_ID, selectHCallback);
  m_hueSlider.setSelection(0, m_curHSV.hue);
  m_hueSlider.setDrawCircle(false);
  m_hueSlider.setDrawVLine(false);

  // the rgb sliders
  m_redSlider.init(hInst, m_colorPickerWindow, "Red", BACK_COL, 24, 256, 306, 10, RED_SLIDER_ID, selectRCallback);
  m_redSlider.setSelection(0, 0);
  m_redSlider.setDrawCircle(false);
  m_redSlider.setDrawVLine(false);

  m_greenSlider.init(hInst, m_colorPickerWindow, "Green", BACK_COL, 24, 256, 338, 10, GREEN_SLIDER_ID, selectGCallback);
  m_greenSlider.setSelection(0, 255);
  m_greenSlider.setDrawCircle(false);
  m_greenSlider.setDrawVLine(false);

  m_blueSlider.init(hInst, m_colorPickerWindow, "Blue", BACK_COL, 24, 256, 370, 10, BLUE_SLIDER_ID, selectBCallback);
  m_blueSlider.setSelection(0, 255);
  m_blueSlider.setDrawCircle(false);
  m_blueSlider.setDrawVLine(false);

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
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  g_pEditor->updateSelectedColors(m_curRGB.raw());
}

void VortexColorPicker::selectH(VSelectBox::SelectEvent sevent, uint32_t h)
{
  selectH(h);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  g_pEditor->updateSelectedColors(m_curRGB.raw());
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
  
void VortexColorPicker::selectR(VSelectBox::SelectEvent sevent, uint32_t r)
{
  selectR(r);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  g_pEditor->updateSelectedColors(m_curRGB.raw());
}

void VortexColorPicker::selectG(VSelectBox::SelectEvent sevent, uint32_t g)
{
  selectG(g);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  g_pEditor->updateSelectedColors(m_curRGB.raw());
}

void VortexColorPicker::selectB(VSelectBox::SelectEvent sevent, uint32_t b)
{
  selectB(b);
  if (sevent != VSelectBox::SelectEvent::SELECT_RELEASE) {
    return;
  }
  g_pEditor->updateSelectedColors(m_curRGB.raw());
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

HBITMAP VortexColorPicker::genRGBBackground(uint32_t width, uint32_t height, int rmult, int gmult, int bmult)
{
  COLORREF *cols = new COLORREF[256 * 256];
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
  }
  m_isOpen = false;
}

// redraw all the components
void VortexColorPicker::triggerRefresh()
{
  m_redSlider.redraw();
  m_greenSlider.redraw();
  m_blueSlider.redraw();
  m_satValBox.redraw();
  m_hueSlider.redraw();
  m_colorPreview.redraw();
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
  // value is inverted idk
  m_curHSV.val = 255 - ival;
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
  m_curRGB.red = 255 - r;
  m_curHSV = m_curRGB;
  refreshColor();
}

void VortexColorPicker::selectG(uint32_t g)
{
  m_curRGB.green = 255 - g;
  m_curHSV = m_curRGB;
  refreshColor();
}

void VortexColorPicker::selectB(uint32_t b)
{
  m_curRGB.blue = 255 - b;
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
  uint32_t rawCol = m_curRGB.raw();
  uint64_t now = GetCurrentTime();
  if (m_colorPreview.getColor() != rawCol && now > m_lastRefresh) {
    m_colorPreview.setColor(rawCol);
    g_pEditor->demoColor(rawCol);
    //g_pEditor->updateSelectedColors(rawCol);
    //g_pEditor->demoColorset();
    // Could actually just update the mode here... However the gloveset is
    // programmed to reset the mode from the beginning when you change it so
    // you won't get a streaming coloret change like you'd expect. If the 
    // editor connection had a new command exposed that told it to simply
    // update the color of the current mode in realtime then we could do that
    //g_pEditor->demoCurMode();
    m_lastRefresh = now;
  }
  triggerRefresh();
}
