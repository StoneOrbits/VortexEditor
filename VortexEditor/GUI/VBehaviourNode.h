#pragma once

#include "VWindow.h"
#include "Behaviours/BehaviourNode.h"
#include "VTextBox.h"
#include "VLabel.h"

class VortexEngine;

class VBehaviourNode : public VWindow
{
public:
    VBehaviourNode();
    virtual ~VBehaviourNode();

    void init(HINSTANCE inst, HWND parent, VortexEngine &engine, BehaviourNode *node, int x, int y);

    void select();
    void deselect();

    virtual void cleanup() override;
    virtual void create() override;
    virtual void paint() override;
    virtual void pressButton(WPARAM wParam, LPARAM lParam) override;
    virtual void releaseButton(WPARAM wParam, LPARAM lParam) override;
    virtual void rightClick(WPARAM wParam, LPARAM lParam) override;
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

    void showContextMenu(int screenX, int screenY, int clientX, int clientY);

private:
    static LRESULT CALLBACK NodeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void ParamChangedCallback(void *arg, VWindow *window);

    void updateParams();

    VortexEngine *m_engine;
    BehaviourNode *m_node;

    VTextBox m_param0;
    VTextBox m_param1;

    bool m_selected;
    bool m_dragging;
    int m_dragOffsetX;
    int m_dragOffsetY;

    friend class VortexBehaviourEditor;
};