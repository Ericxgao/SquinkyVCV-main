#include "InputControls.h"


void InputPopupMenuParamWidget::draw(const Widget::DrawArgs &args)
{
    if (enabled) {
        PopupMenuParamWidget::draw(args);
    }
}

void InputPopupMenuParamWidget::setValue(float v)
{
    int i = int(std::round(v));
    if (i < 0 || i >= int(longLabels.size())) {
        WARN("popup set value illegal");
        assert(false);
        return;
    }
    this->text = longLabels[i];
    if (label) {
        label->text = longLabels[i];
    }
    if (getParamQuantity()) {
        getParamQuantity()->setValue(v);
    }
}

float InputPopupMenuParamWidget::getValue() const
{
    if (getParamQuantity()) {
        return getParamQuantity()->getValue();
    }
    
    // Fallback behavior for when paramQuantity is not set
    int index = 0;
    for (auto label : longLabels) {
        if (this->text == label) {
            return index;
        }
        ++index;
    }
    assert(false);
    return 0;
}

void InputPopupMenuParamWidget::setCallback(std::function<void(void)> cb)
{
    callback = cb;
    // Setup notification callback that will be called when knob value changes
    setNotificationCallback([this](int) {
        if (this->callback) {
            this->callback();
        }
    });
}

void InputPopupMenuParamWidget::enable(bool b)
{
    enabled = b;
    if (knob) {
        knob->visible = b;
    }
    if (label) {
        label->visible = b;
    }
}


void CheckBox::setCallback(std::function<void(void)> cb)
{
    callback = cb;
}

float CheckBox::getValue() const
{
    return value ? 1.f : 0.f;
}

void CheckBox::setValue(float v)
{
    value = v;
}

void CheckBox::draw(const Widget::DrawArgs &args)
{
    NVGcontext *const ctx = args.vg;

    if (!enabled) {
        return;
    }

    nvgShapeAntiAlias(ctx, true);
    drawBox(ctx);
    if (value) {
        drawX(ctx);
    }
}

void CheckBox::enable(bool _enabled)
{
    enabled = _enabled;
}

void CheckBox::drawBox(NVGcontext* ctx)
{
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, 0, 0);
    nvgLineTo(ctx, box.size.x, 0);
    nvgLineTo(ctx, box.size.x, box.size.y);
    nvgLineTo(ctx, 0, box.size.y);
    nvgLineTo(ctx, 0, 0);
    nvgStrokeColor(ctx, UIPrefs::TIME_LABEL_COLOR);
    //  nvgStrokePaint
    nvgStrokeWidth(ctx, 1);
    nvgStroke(ctx);
}

void CheckBox::drawX(NVGcontext* ctx)
{
    nvgBeginPath(ctx);
    nvgStrokeColor(ctx, UIPrefs::TIME_LABEL_COLOR);
    nvgStrokeWidth(ctx, 1);

    nvgMoveTo(ctx, 0, 0);
    nvgLineTo(ctx, box.size.x,  box.size.y);

    nvgMoveTo(ctx, box.size.x, 0);
    nvgLineTo(ctx, 0,  box.size.y);
 
    nvgStroke(ctx);
}

void CheckBox::onDragDrop(const ::rack::event::DragDrop& e)
{
    if (e.origin == this) {
		::rack::widget::Widget::ActionEvent eAction;
		onAction(eAction);
	}
}

void CheckBox::onAction(const ::rack::widget::Widget::ActionEvent& e)
{
    value = !value;
    if (callback) {
        callback();
    }
}