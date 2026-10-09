#include <format>

#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"

namespace {

const char *toneClass(int toneIndex) noexcept
{
    switch(toneIndex % 3) {
    case 0:  return "bit-tone-blue";
    case 1:  return "bit-tone-green";
    default: return "bit-tone-purple";
    }
}

} // namespace

BinaryFieldWidget::BinaryFieldWidget(const InstField &field, int toneIndex)
{
    auto *pWrap= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    PRoot_     = pWrap;
    pWrap->add_css_class("bit-field-wrap");
    pWrap->set_hexpand(false);
    pWrap->set_valign(Gtk::Align::START);
    pWrap->set_halign(Gtk::Align::FILL);

    const auto RANGE=
        (field.startBit_ == field.endBit_)
            ? std::format("{}", field.startBit_)
            : std::format("{} : {}", field.endBit_, field.startBit_);

    auto *pRange= Gtk::make_managed<Gtk::Label>(RANGE);
    pRange->add_css_class("bit-range-label");
    pRange->set_halign(Gtk::Align::CENTER);
    pRange->set_can_target(false);
    pWrap->append(*pRange);

    auto *pBlock= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    pBlock->add_css_class("bit-field-container");
    pBlock->add_css_class(toneClass(toneIndex));
    pBlock->set_halign(Gtk::Align::FILL);
    pBlock->set_can_target(false);

    const auto CAPTION= field.desc_.empty() ? field.name_ : field.desc_;
    auto *pName       = Gtk::make_managed<Gtk::Label>(Glib::ustring(CAPTION.data(), CAPTION.size()));
    pName->add_css_class("field-name-label");
    pName->set_halign(Gtk::Align::CENTER);
    pName->set_can_target(false);
    pBlock->append(*pName);

    auto *pBits= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 2);
    pBits->set_halign(Gtk::Align::CENTER);
    pBits->set_can_target(false);

    const int WIDTH= field.endBit_ - field.startBit_ + 1;
    controlLabels_.reserve(static_cast<size_t>(WIDTH));
    for(int i= 0; i < WIDTH; ++i) {
        auto *pBit= Gtk::make_managed<Gtk::Label>("0");
        pBit->set_width_chars(1);
        pBit->set_halign(Gtk::Align::CENTER);
        pBit->set_valign(Gtk::Align::CENTER);
        pBit->add_css_class("binary-bit");
        pBit->set_can_target(false);
        controlLabels_.emplace_back(pBit);
        pBits->append(*pBit);
    }

    pBlock->append(*pBits);
    pWrap->append(*pBlock);
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
        const auto BIT= (fieldValue >> (N - 1 - i)) & 1U;
        controlLabels_[i]->set_text(BIT != 0U ? "1" : "0");
        IHoverWidget::SetCSSClass(controlLabels_[i], "bit-one", BIT != 0U);
    }
}
