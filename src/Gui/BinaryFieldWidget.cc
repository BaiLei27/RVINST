#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"

BinaryFieldWidget::BinaryFieldWidget(const InstField &field, int &nibbleIndex)
{
    auto *pBox= Gtk::make_managed<Gtk::Overlay>();
    PRoot_    = pBox;
    pBox->add_css_class("bit-field-container");
    pBox->set_hexpand(false);
    pBox->set_valign(Gtk::Align::CENTER);

    auto *pBits= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    pBits->set_halign(Gtk::Align::CENTER);
    pBits->set_can_target(false);

    const int WIDTH= field.endBit_ - field.startBit_ + 1;
    controlLabels_.reserve(static_cast<size_t>(WIDTH));
    for(int i= 0; i < WIDTH; ++i) {
        auto *pBit= Gtk::make_managed<Gtk::Label>("0");
        pBit->set_width_chars(1);
        pBit->set_halign(Gtk::Align::CENTER);
        pBit->set_valign(Gtk::Align::CENTER);
        pBit->set_size_request(18, 30);
        pBit->add_css_class("binary-bit");
        pBit->set_can_target(false);
        if(nibbleIndex > 0 && nibbleIndex % 4 == 0)
            pBit->set_margin_start(8);
        controlLabels_.emplace_back(pBit);
        pBits->append(*pBit);
        ++nibbleIndex;
    }

    auto caption= field.desc_.empty() ? field.name_ : field.desc_;
    auto *pName = Gtk::make_managed<Gtk::Label>(Glib::ustring(caption.data(), caption.size()));
    pName->add_css_class("field-label");
    pName->set_halign(Gtk::Align::CENTER);
    pName->set_valign(Gtk::Align::START);
    pName->set_can_target(false);

    pBox->set_child(*pBits);
    pBox->add_overlay(*pName);
    pBox->set_measure_overlay(*pName, false);
}

void BinaryFieldWidget::onHoverChanged(bool on)
{
    IHoverWidget::SetHighlightedAll(relatedAsm_, on);
    IHoverWidget::SetHighlightedAll(relatedBinary_, on);
}

void BinaryFieldWidget::SetupHover(std::string_view name,
                                   const util::InstFormatView &view,
                                   InstCommST::BinaryFieldWidgetMap_u &binaryFieldWidgets,
                                   InstCommST::AsmMnemonicWidgetMap_u &asmFieldWidgets)
{
    const auto &rels= util::FIND_FIELD_REL(view.binaryRel_, name);
    IHoverWidget::CollectRelated(asmFieldWidgets, rels, relatedAsm_);
    IHoverWidget::CollectRelated(binaryFieldWidgets, rels, relatedBinary_, this);
    attachMotion();
}

void BinaryFieldWidget::UpdateBits(uint32_t fieldValue)
{
    const auto N= controlLabels_.size();
    for(size_t i= 0; i < N; ++i) {
        auto bit= (fieldValue >> (N - 1 - i)) & 1U;
        controlLabels_[i]->set_text(bit != 0U ? "1" : "0");
    }
}
