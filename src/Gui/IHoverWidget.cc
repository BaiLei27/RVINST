#include "Gui/IHoverWidget.hh"

void IHoverWidget::SetCSSClass(Gtk::Widget *pWidget, const char *pCls, bool on) noexcept
{
    if(!pWidget) return;

    on ? pWidget->add_css_class(pCls) : pWidget->remove_css_class(pCls);
}

void IHoverWidget::SetHighlighted(IHoverWidget *pWidget, bool on) noexcept
{
    if(!pWidget) return;

    on ? pWidget->Highlight() : pWidget->Unhighlight();
}

void IHoverWidget::Highlight()
{
    SetCSSClass(PRoot_, "highlighted", true);
}

void IHoverWidget::Unhighlight()
{
    SetCSSClass(PRoot_, "highlighted", false);
}

Gtk::Widget *IHoverWidget::GetRoot() const noexcept
{
    return PRoot_;
}

void IHoverWidget::attachMotion()
{
    if(!PRoot_) return;

    PMotion_= Gtk::EventControllerMotion::create();
    PMotion_->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    PMotion_->signal_enter().connect([this](double, double) { applyHover(true); });
    PMotion_->signal_leave().connect([this] { applyHover(false); });
    PRoot_->add_controller(PMotion_);
}

void IHoverWidget::applyHover(bool on)
{
    if(Hover_ == on) return;

    Hover_= on;
    onHoverChanged(on);
}
