#include "VPatternListBox.h"
#include <CommCtrl.h>
#include <Windowsx.h>

#define WC_PATTERN_LIST_BOX "VPatternListBox"

// Constructor with initialization list
VPatternListBox::VPatternListBox() 
    : VWindow(), m_scrollPos(0), m_callback(nullptr) {}

// Delegate constructor
VPatternListBox::VPatternListBox(HINSTANCE hInstance, VWindow &parent, uint32_t width, uint32_t height, 
                                 uint32_t x, uint32_t y, uintptr_t menuID, VWindowCallback callback) 
    : VPatternListBox() {
    init(hInstance, parent, width, height, x, y, menuID, callback);
}

// Destructor
VPatternListBox::~VPatternListBox() {
    cleanup();
}

// Initialize the VPatternListBox
void VPatternListBox::init(HINSTANCE hInstance, VWindow &parent, uint32_t width, uint32_t height, 
                           uint32_t x, uint32_t y, uintptr_t menuID, VWindowCallback callback) {
    m_callback = callback;

    m_hwnd = CreateWindow(WC_PATTERN_LIST_BOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                          x, y, width, height, parent.hwnd(), (HMENU)menuID, hInstance, nullptr);
    if (!m_hwnd) {
        MessageBox(nullptr, "Failed to create pattern list box", "Error", MB_OK);
        return;
    }

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

// Clean up resources
void VPatternListBox::cleanup() {
    // Cleanup logic here
}

// Handle creation logic
void VPatternListBox::create() {
    // Additional setup on creation
}

// Handle painting of the list box
void VPatternListBox::paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    drawPatternStrips(hdc);

    EndPaint(m_hwnd, &ps);
}

// Add a new pattern strip to the list
void VPatternListBox::addPatternStrip(std::shared_ptr<VPatternStrip> patternStrip) {
    m_patternStrips.push_back(patternStrip);
    // Update view
}

// Remove a pattern strip from the list
void VPatternListBox::removePatternStrip(size_t index) {
    if (index < m_patternStrips.size()) {
        m_patternStrips.erase(m_patternStrips.begin() + index);
        // Update view
    }
}

// Clear all pattern strips
void VPatternListBox::clearPatternStrips() {
    m_patternStrips.clear();
    // Update view
}

// Draw all pattern strips within the list box
void VPatternListBox::drawPatternStrips(HDC hdc) {
    int yOffset = 0;
    for (auto &strip : m_patternStrips) {
        if (yOffset - m_scrollPos >= -40 && yOffset - m_scrollPos < m_height) {
            strip->draw(hdc, 0, yOffset - m_scrollPos, 200, 32);
        }
        yOffset += 40;
    }
}

// Command handling (placeholder for future logic)
void VPatternListBox::command(WPARAM wParam, LPARAM lParam) {
    // Implement as needed
}

// Handle button press events (placeholder for future logic)
void VPatternListBox::pressButton(WPARAM wParam, LPARAM lParam) {
    // Implement as needed
}

// Handle button release events (placeholder for future logic)
void VPatternListBox::releaseButton(WPARAM wParam, LPARAM lParam) {
    // Implement as needed
}
