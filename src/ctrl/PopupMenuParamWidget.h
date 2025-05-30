#pragma once

#include "rack.hpp"
#include "SqUI.h"
#include "ctrl/SqWidgets.h"

#include <random>
#include <functional>

/**
 * UI Widget that does:
 *  functions as a parameter
 *  pops up a menu of discrete choices.
 *  displays the current choice
 */

class PopupMenuParamWidget : public ::rack::app::ParamWidget {
public:
    std::vector<std::string> longLabels;
    std::vector<std::string> shortLabels;
    std::string text = {"pop widget default"};
    RoganSLBlue30* knob = nullptr; // Added knob pointer

    PopupMenuParamWidget() {
        // Create and initialize the knob
        knob = new RoganSLBlue30();
        knob->box.pos = Vec(0, 0);
        addChild(knob);
    }

    ~PopupMenuParamWidget() {
        // The widget tree will delete knob automatically
    }

    void step() override {
        ParamWidget::step();
        
        // // Make sure knob shows the same parameter value as the widget
        // if (knob && this->getParamQuantity()) {
        //     if (!knob->getParamQuantity()) {
        //         // Connect knob's parameter to the widget's parameter
        //         knob->paramQuantity = this->paramQuantity;
        //     }
        // }
    }

    /** Creator must call this function before adding widget to the stage.
     * These are the string that will show in the dropdown, and possibly
     * in the text control while the menu is not open.
     */
    void setLabels(std::vector<std::string> l) {
        longLabels = l;
        ::rack::widget::Widget::ChangeEvent e;
        onChange(e);
    }

    /**
     * Short labels, if present, will be used for the text shown when menu is closed.
     * Short labels are optional.
     * Providing short labels can save panel space, while dropdown shows the full text.
     */
    void setShortLabels(std::vector<std::string> l) {
        shortLabels = l;
        ::rack::widget::Widget::ChangeEvent e;
        onChange(e);
    }

    using NotificationCallback = std::function<void(int index)>;
    void setNotificationCallback(NotificationCallback);

    // input is parameter value (quantized), output is control index/
    using IndexToValueFunction = std::function<float(int index)>;
    using ValueToIndexFunction = std::function<int(float value)>;
    void setIndexToValueFunction(IndexToValueFunction);
    void setValueToIndexFunction(ValueToIndexFunction);

    void drawLayer(const DrawArgs &arg, int layer) override;
    void onButton(const ::rack::widget::Widget::ButtonEvent &e) override;
    void onChange(const ::rack::widget::Widget::ChangeEvent &e) override;
    void onAction(const ::rack::widget::Widget::ActionEvent &e) override;
  
    friend class PopupMenuItem;

private:
    NotificationCallback optionalNotificationCallback = {nullptr};
    IndexToValueFunction optionalIndexToValueFunction = {nullptr};
    ValueToIndexFunction optionalValueToIndexFunction = {nullptr};
    int curIndex = 0;

    void randomize();
    std::string getShortLabel(unsigned int index);
};

inline std::string PopupMenuParamWidget::getShortLabel(unsigned int index) {
    std::string ret;

    // if index is out of the range of long labels, ignore it.
    if (index < longLabels.size()) {
        // If we have a long label, use it as a fall-back
        ret = longLabels[index];
        if (index < shortLabels.size()) {
            // but if there is a short label, use it.
            ret = shortLabels[index];
        }
    }
    return ret;
} 

inline void PopupMenuParamWidget::randomize() {
    if (getParamQuantity() && getParamQuantity()->isBounded()) {
        float value = rack::math::rescale(rack::random::uniform(), 0.f, 1.f, getParamQuantity()->getMinValue(), getParamQuantity()->getMaxValue());
        value = std::round(value);
        getParamQuantity()->setValue(value);
    }
}

inline void PopupMenuParamWidget::setNotificationCallback(NotificationCallback callback) {
    optionalNotificationCallback = callback;
}

inline void PopupMenuParamWidget::setIndexToValueFunction(IndexToValueFunction fn) {
    optionalIndexToValueFunction = fn;
}

inline void PopupMenuParamWidget::setValueToIndexFunction(ValueToIndexFunction fn) {
    optionalValueToIndexFunction = fn;
}

inline void PopupMenuParamWidget::onChange(const ::rack::widget::Widget::ChangeEvent &e) {
    if (!this->getParamQuantity()) {
        return;  // no module. Probably in the module browser.
    }

    // process ourself to update the text label
    int index = (int)std::round(this->getParamQuantity()->getValue());
    if (optionalValueToIndexFunction) {
        float value = this->getParamQuantity()->getValue();
        index = optionalValueToIndexFunction(value);
    }

    auto label = getShortLabel(index);
    if (!label.empty()) {
        this->text = label;
        curIndex = index;  // remember it
    }

    // Delegate to base class to change param value
    ParamWidget::onChange(e);
    if (optionalNotificationCallback) {
        optionalNotificationCallback(index);
    }
    
    // Update knob value when parameter changes
    if (knob && knob->getParamQuantity() && knob->getParamQuantity() != getParamQuantity()) {
        knob->getParamQuantity()->setValue(getParamQuantity()->getValue());
    }
}

inline void PopupMenuParamWidget::drawLayer(const DrawArgs &args, int layer) {
    if (layer == 1) {
        BNDwidgetState state = BND_DEFAULT;
        
        // Calculate positions for button and knob
        float knobSize = std::min(30.0f, box.size.y - 4); // Ensure knob fits height with small margin
        float buttonWidth = box.size.x - knobSize - 5; // 5px spacing
        
        // Draw the popup button on the left side
        bndChoiceButton(args.vg, 0.0, 0.0, buttonWidth, box.size.y, BND_CORNER_NONE, state, -1, text.c_str());
        
        // Position the knob on the right side
        if (knob) {
            knob->box.size = Vec(knobSize, knobSize);
            knob->box.pos = Vec(buttonWidth + 5, (box.size.y - knobSize) / 2);
        }
    }
    ParamWidget::drawLayer(args, layer);
}

inline void PopupMenuParamWidget::onButton(const ::rack::widget::Widget::ButtonEvent &e) {
    if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)) {
        // Check if click is on button area, not on knob
        float knobSize = std::min(30.0f, box.size.y - 4);
        float buttonWidth = box.size.x - knobSize - 5;
        
        // Only open menu if click is on button area
        if (e.pos.x <= buttonWidth) {
            // remember which param is touched, so mapping can work.
            if (module) {
                APP->scene->rack->setTouchedParam(this);
            }
            ::rack::widget::Widget::ActionEvent ea;
            onAction(ea);
            sq::consumeEvent(&e, this);
        }
        // If click is on knob area, let the knob handle it
        // The event is not consumed here so it propagates to the knob
    }
}

class PopupMenuItem : public ::rack::ui::MenuItem {
public:
    /**
     * param index is the menu item index, but also the
     *  parameter value.
     */
    PopupMenuItem(int index, PopupMenuParamWidget *inParent) : index(index), parent(inParent) {
        text = parent->longLabels[index];
    }

    const int index;
    PopupMenuParamWidget *const parent;

    void onAction(const ::rack::widget::Widget::ActionEvent &e) override {
        parent->text = this->text;
        ::rack::widget::Widget::ChangeEvent ce;
        if (parent->getParamQuantity()) {
            float newValue = index;
            const float oldValue =  parent->getParamQuantity()->getValue();
            if (parent->optionalIndexToValueFunction) {
                newValue = parent->optionalIndexToValueFunction(index);
            }
            parent->getParamQuantity()->setValue(newValue);
     
            // Push ParamChange history action so user may undo this change
			::rack::history::ParamChange* h = new ::rack::history::ParamChange;
			h->name = "change menu";
			h->moduleId = parent->module->id;
			h->paramId = parent->paramId;
			h->oldValue = oldValue;
			h->newValue = newValue;
			APP->history->push(h);
        }
        parent->onChange(ce);
    }
};

inline void PopupMenuParamWidget::onAction(const ::rack::widget::Widget::ActionEvent &e) {
    ::rack::ui::Menu *menu = ::rack::createMenu();

    // is choice button the right base class?
    menu->box.pos = getAbsoluteOffset(::rack::math::Vec(0, this->box.size.y)).round();
    menu->box.size.x = box.size.x;
    {
        for (int i = 0; i < (int)longLabels.size(); ++i) {
            menu->addChild(new PopupMenuItem(i, this));
        }
    }
}
