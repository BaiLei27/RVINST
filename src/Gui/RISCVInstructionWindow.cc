#include <array>
#include <print>
#include <sstream>

#include "Gui/RISCVInstructionWindow.hh"
#include "Util/InputParse.hpp"

namespace {

constexpr std::array G_kFormatOrder {
    InstFormat::R,
    InstFormat::I,
    InstFormat::S,
    InstFormat::B,
    InstFormat::U,
    InstFormat::J,
};

} // namespace

RISCVInstructionWindow::RISCVInstructionWindow()
    : insEntry_(Gtk::make_managed<Gtk::Entry>()),
      insButtonParse_(Gtk::make_managed<Gtk::Button>("Parse Instruction")),
      insTextView_(Gtk::make_managed<Gtk::TextView>()),
      pSettingsBtn_(Gtk::make_managed<Gtk::Button>()),
      uiContainer_(Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10))
{
    set_title("RISC-V Instruction Encoder/Decoder");
    set_default_size(900, 300);

    uiContainer_->set_margin(15);

    insEntry_->set_placeholder_text("Hex (0x33), binary (0b110011 or 32 bits), or assembly (add x0,x0,x0)");
    insEntry_->set_hexpand(true);
    insEntry_->signal_activate().connect(sigc::mem_fun(*this, &RISCVInstructionWindow::onInsButtonParseClicked));

    pEntryRow_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
    pEntryRow_->append(*insEntry_);

    pSettingsBtn_->set_icon_name("view-more-symbolic");
    pSettingsBtn_->set_tooltip_text("ABI / ISA settings");
    setupSettingsPopover();
    pEntryRow_->append(*pSettingsBtn_);

    insButtonParse_->signal_clicked().connect(sigc::mem_fun(*this, &RISCVInstructionWindow::onInsButtonParseClicked));

    uiContainer_->append(*pEntryRow_);
    uiContainer_->append(*insButtonParse_);

    insTextView_->set_margin(10);
    insTextView_->set_vexpand(true);
    insTextView_->set_hexpand(true);
    insTextView_->set_editable(false);
    insTextView_->set_cursor_visible(false);
    loadCSSFromFile();

    initInstFormatUI();
    uiContainer_->append(*insTextView_);
    set_child(*uiContainer_);
}

InstFormatUI *RISCVInstructionWindow::uiFor(InstFormat fmt) const noexcept
{
    const auto IDX= static_cast<int>(fmt);
    if(IDX < 0 || static_cast<size_t>(IDX) >= formatUi_.size()) return nullptr;

    return formatUi_[static_cast<size_t>(IDX)];
}

void RISCVInstructionWindow::initInstFormatUI()
{
    const auto PUT_TO_ENTRY= [this](const std::string &content) {
        if(insEntry_ && insEntry_->get_buffer()) insEntry_->get_buffer()->set_text(content);
    };

    for(const auto &fmt: G_kFormatOrder) {
        auto *pUi= Gtk::make_managed<InstFormatUI>(fmt);
        pUi->set_visible(false);
        pUi->signalOutput_.connect(PUT_TO_ENTRY);
        formatUi_[static_cast<size_t>(fmt)]= pUi;
        uiContainer_->append(*pUi);
    }
}

void RISCVInstructionWindow::hideAllTypeUI()
{
    for(auto *pUi: formatUi_) {
        if(pUi) pUi->set_visible(false);
    }
}

void RISCVInstructionWindow::onInsButtonParseClicked()
{
    const auto TEXT= insEntry_->get_text();
    if(TEXT.empty()) {
        showError("invalid input: empty");
        return;
    }

    std::string inputStr(TEXT);
    try {
        inst_.reset();
        const auto PARSED= util::ClassifyInstInput(inputStr);
        if(PARSED.kind_ == util::InputKind::ASSEMBLY) {
            inst_= std::make_unique<Instruction>(inputStr, hasSetABI_);
        } else {
            inst_= std::make_unique<Instruction>(PARSED.word_, hasSetABI_);
        }

        if(!inst_->Decode()) throw std::invalid_argument("Failed to decode instruction");

        showInsResult(*inst_);
    } catch(const std::invalid_argument &e) {
        showError(std::string("invalid input: ") + e.what());
    } catch(const std::out_of_range &) {
        showError("invalid input: value must fit in 32 bits");
    } catch(const std::exception &e) {
        showError(std::string("invalid input: ") + e.what());
    } catch(...) {
        showError("invalid input: unknown error");
    }

    insEntry_->grab_focus();
}

void RISCVInstructionWindow::showInsResult(Instruction &inst)
{
    auto buffer= insTextView_->get_buffer();
    if(!buffer) {
        std::println(stderr, "Error: TextView buffer is null.");
        return;
    }

    hideAllTypeUI();
    const auto *pType= inst.GetTypePtr();
    auto *pCurrUi    = pType ? uiFor(pType->GetInstFormat()) : nullptr;
    if(!pCurrUi) {
        showError("invalid input: unsupported instruction format");
        return;
    }
    pCurrUi->set_visible(true);

    std::ostringstream oss;
    oss << "Format          = " << inst.GetFormat() << '\n'
        << "Instruction set = " << inst.GetXLEN() << '\n';
    buffer->set_text(oss.str());
    pCurrUi->UpdateDisplay(inst);
}

void RISCVInstructionWindow::showError(const std::string &message)
{
    hideAllTypeUI();
    inst_.reset();
    if(auto buffer= insTextView_->get_buffer()) buffer->set_text("Error: " + message);

    std::println(stderr, "Error: {}", message);
}

void RISCVInstructionWindow::refreshAssemblyForAbiChange()
{
    if(!inst_) return;

    auto val= static_cast<uint32_t>(*inst_);
    inst_   = std::make_unique<Instruction>(val, hasSetABI_);
    inst_->Decode() ? showInsResult(*inst_) : showError("invalid input: failed to decode after ABI change");
}

void RISCVInstructionWindow::setupSettingsPopover()
{
    pSettingsPopover_= Gtk::make_managed<Gtk::Popover>();
    pSettingsPopover_->set_has_arrow(true);
    pSettingsPopover_->set_parent(*pSettingsBtn_);

    auto *pPopoverBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    pPopoverBox->set_margin(12);

    auto *pAbiRow  = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto *pAbiLabel= Gtk::make_managed<Gtk::Label>("ABI");
    pAbiLabel->set_halign(Gtk::Align::START);
    pAbiRow->append(*pAbiLabel);

    pAbiSwitch_= Gtk::make_managed<Gtk::Switch>();
    pAbiSwitch_->set_active(hasSetABI_);
    pAbiSwitch_->set_halign(Gtk::Align::END);
    pAbiSwitch_->property_active().signal_changed().connect([this] {
        hasSetABI_= pAbiSwitch_->get_active();
        refreshAssemblyForAbiChange();
    });
    pAbiRow->append(*pAbiSwitch_);
    pPopoverBox->append(*pAbiRow);

    auto *pIsaRow  = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto *pIsaLabel= Gtk::make_managed<Gtk::Label>("ISA");
    pIsaLabel->set_halign(Gtk::Align::START);
    pIsaRow->append(*pIsaLabel);

    pIsaMenuBtn_= Gtk::make_managed<Gtk::MenuButton>();
    pIsaMenuBtn_->set_label("AUTO");
    pIsaMenuBtn_->set_hexpand(true);
    pIsaMenuBtn_->set_halign(Gtk::Align::END);

    auto *pIsaPopover= Gtk::make_managed<Gtk::Popover>();
    auto *pIsaChoices= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
    pIsaChoices->set_margin(6);

    static constexpr std::pair<const char *, int> K_ISA_OPTS[] {
        { "AUTO",   0 },
        { "RV32I",  1 },
        { "RV64I",  2 },
        { "RV128I", 3 },
    };

    for(const auto &[label, idx]: K_ISA_OPTS) {
        auto *pOptBtn= Gtk::make_managed<Gtk::Button>(label);
        pOptBtn->signal_clicked().connect([this, pIsaPopover, label, idx] {
            selectedIsaIndex_= idx;
            if(pIsaMenuBtn_) pIsaMenuBtn_->set_label(label);

            pIsaPopover->popdown();

            if(pSettingsPopover_) pSettingsPopover_->popdown();
        });
        pIsaChoices->append(*pOptBtn);
    }

    pIsaPopover->set_child(*pIsaChoices);
    pIsaMenuBtn_->set_popover(*pIsaPopover);
    pIsaRow->append(*pIsaMenuBtn_);
    pPopoverBox->append(*pIsaRow);

    pSettingsPopover_->set_child(*pPopoverBox);
    pSettingsBtn_->signal_clicked().connect([this] { pSettingsPopover_->popup(); });
}

void RISCVInstructionWindow::loadCSSFromFile()
{
    try {
        auto cssProvider= Gtk::CssProvider::create();
        cssProvider->load_from_file(Gio::File::create_for_path(CSS_FILE_PATH));
        if(auto display= Gdk::Display::get_default()) {
            Gtk::StyleContext::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    } catch(const Glib::Error &ex) {
        std::println(stderr, "CSS error: {}", ex.what());
    }
}
