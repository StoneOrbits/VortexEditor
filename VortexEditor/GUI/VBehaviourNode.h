#pragma once

#include "VWindow.h"
#include "Behaviours/BehaviourNode.h"

class VortexEngine;

class VBehaviourNode : public VWindow
{
public:
    VBehaviourNode();
    virtual ~VBehaviourNode();

    void init(HINSTANCE inst, HWND parent, VortexEngine &engine, BehaviourNode *node, int x, int y);

    virtual void cleanup() override;
    virtual void create() override;
    virtual void paint() override;
    virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
    virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;
    virtual INT_PTR controlColor(WPARAM wParam, LPARAM lParam) override;
    virtual void mouseMove(WPARAM wParam, LPARAM lParam) override;
    virtual void command(WPARAM wParam, LPARAM lParam) override;

    BehaviourNode *node();
    int outputSocketX();
    int outputSocketY();
    int inputSocketX(int i);
    int inputSocketY(int i);
    const char *nodeTypeName(Behaviours::NodeType t);
    COLORREF nodeColor();

private:
    static LRESULT CALLBACK NodeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void updateParams();

    VortexEngine *m_engine;
    BehaviourNode *m_node;

    HWND m_param0;
    HWND m_param1;

    bool m_dragging;
    int m_dragOffsetX;
    int m_dragOffsetY;
};