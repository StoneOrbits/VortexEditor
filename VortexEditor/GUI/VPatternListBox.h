#pragma once

#include "VWindow.h"
#include "VPatternStrip.h" // Ensure this includes the definition of VPatternStrip
#include <vector>
#include <memory>

class VPatternListBox : public VWindow {
public:
    VPatternListBox();
    VPatternListBox(HINSTANCE hInstance, VWindow& parent, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uintptr_t menuID, VWindowCallback callback);
    virtual ~VPatternListBox();

    void init(HINSTANCE hInstance, VWindow& parent, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uintptr_t menuID, VWindowCallback callback);
    virtual void cleanup() override;

    // Override these methods for custom behavior
    virtual void create() override;
    virtual void paint() override;
    virtual void command(WPARAM wParam, LPARAM lParam) override;
    virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
    virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;

    // Methods to manage pattern strips
    void addPatternStrip(std::shared_ptr<VPatternStrip> patternStrip);
    void removePatternStrip(size_t index);
    void clearPatternStrips();

protected:
    std::vector<std::shared_ptr<VPatternStrip>> m_patternStrips; // Container for pattern strips
    uint32_t m_scrollPos;
    uint32_t m_height;

private:
    VWindowCallback m_callback;
    void drawPatternStrips(HDC hdc); // Helper function to draw pattern strips
};
