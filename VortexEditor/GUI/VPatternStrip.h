#pragma once

#include "VWindow.h"
#include "VLabel.h"
#include "VortexLib.h"
#include <deque>
#include <map>
#include <string>

class VPatternStrip : public VWindow {
public:
    enum SelectEvent {
        SELECT_LEFT_CLICK,
        SELECT_CTRL_LEFT_CLICK,
        SELECT_SHIFT_LEFT_CLICK,
        SELECT_RIGHT_CLICK,
    };

    typedef void (*VPatternStripCallback)(void* arg, VPatternStrip* select, SelectEvent sevent);

    VPatternStrip();
    VPatternStrip(HINSTANCE hinstance, VWindow& parent, const std::string& title, 
        COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
        uint32_t linewidth, const json& modeData, uintptr_t menuID, VPatternStripCallback callback);
    virtual ~VPatternStrip();

    void init(HINSTANCE hinstance, VWindow& parent, const std::string& title, 
        COLORREF backcol, uint32_t width, uint32_t height, uint32_t x, uint32_t y,
        uint32_t linewidth, const json& modeData, uintptr_t menuID, VPatternStripCallback callback);
    void cleanup();

    void loadJson(const json &modeData);

    void run();
    void create() override;
    void paint() override;
    void command(WPARAM wParam, LPARAM lParam) override;
    void pressButton(WPARAM wParam, LPARAM lParam) override;
    void releaseButton(WPARAM wParam, LPARAM lParam) override;
    void rightButtonPress();

    void clear();
    void setColor(uint32_t col);
    uint32_t getColor() const;
    std::string getColorName() const;
    void setColor(const std::string& name);

    bool isActive() const;
    void setActive(bool active);
    bool isSelected() const;
    void setSelected(bool selected);

    void setLabelEnabled(bool enabled);
    void setSelectable(bool selectable);

    // New method for external drawing
    void draw(HDC hdc, int x, int y, int width, int height);

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void registerWindowClass(HINSTANCE hInstance, COLORREF backcol);
    static WNDCLASS m_wc;

    static DWORD __stdcall runThread(void *arg);

    Vortex m_vortex;
    HANDLE m_runThread;
    VLabel m_stripLabel;
    VPatternStripCallback m_callback;
    bool m_active;
    bool m_selected;
    bool m_selectable;

    std::deque<RGBColor> m_colorSequence;
    uint32_t m_lineWidth;
    uint32_t m_numSlices;

    // Backbuffer management
    HDC m_backbufferDC;
    HBITMAP m_backbuffer;
    HBITMAP m_oldBitmap;
    uint32_t m_backbufferWidth;
    uint32_t m_backbufferHeight;
    uint32_t m_scrollOffset;

    void createBackBuffer(HDC hdc, uint32_t width, uint32_t height);
    void destroyBackBuffer();
    void updateVisuals(); // New method to update the backbuffer visuals
    void drawToBackBuffer();
};