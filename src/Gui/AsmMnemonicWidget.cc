#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"

AsmMnemonicWidget::AsmMnemonicWidget(std::string_view token, Gtk::Box *pParentAsmBox)
{
    if(!pParentAsmBox) return;

    auto *pBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    PRoot_    = pBox;
    pBox->add_css_class("instruction-container");
    pLabel_= Gtk::make_managed<Gtk::Label>(Glib::ustring(token.data(), token.size()));
    pLabel_->set_can_target(false);
    if(token == "mnemonic") {
        pLabel_->set_margin_end(8);
        pLabel_->add_css_class("instruction-label");
    } else if(token != ",") {
        pLabel_->add_css_class("register-label");
    }
    pBox->append(*pLabel_);
    pParentAsmBox->append(*pBox);
}

void AsmMnemonicWidget::onHoverChanged(bool on)
{
    IHoverWidget::SetHighlightedAll(relatedBinary_, on);
}

Gtk::Label *AsmMnemonicWidget::GetLabel() const noexcept { return pLabel_; }

void AsmMnemonicWidget::SetupHover(std::string_view name,
                                   const util::InstFormatView &view,
                                   InstCommST::BinaryFieldWidgetMap_u &binaryFieldWidgets)
{
    IHoverWidget::CollectRelated(binaryFieldWidgets, util::FIND_FIELD_REL(view.asmRel_, name), relatedBinary_);
    attachMotion();
}
