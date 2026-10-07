#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <print>

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

constexpr int G_kMaxInputHistory   = 32;
constexpr int G_kHistoryVisibleRows= 8;
constexpr int G_kHistoryRowHeight  = 28;

} // namespace

RISCVInstructionWindow::RISCVInstructionWindow()
    : insEntry_(Gtk::make_managed<Gtk::Entry>()),
      insButtonParse_(Gtk::make_managed<Gtk::Button>("Parse Instruction")),
      insTextView_(Gtk::make_managed<Gtk::TextView>()),
      pSettingsBtn_(Gtk::make_managed<Gtk::Button>()),
      uiContainer_(Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10))
{
    set_title("RISC-V Instruction Encoder/Decoder");
    set_default_size(900, 480);

    uiContainer_->set_margin(15);

    insEntry_->set_placeholder_text("Hex (0x33), binary (0b110011 or 32 bits), or assembly (add x0,x0,x0)");
    insEntry_->set_hexpand(true);
    insEntry_->signal_activate().connect(sigc::mem_fun(*this, &RISCVInstructionWindow::onInsButtonParseClicked));
    setupInputHistory();

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
    const int IDX= static_cast<int>(fmt);
    if(IDX < 0 || IDX >= formatUi_.size()) return nullptr;

    return formatUi_[IDX];
}

void RISCVInstructionWindow::initInstFormatUI()
{
    pFormatStack_= Gtk::make_managed<Gtk::Stack>();
    pFormatStack_->set_transition_type(Gtk::StackTransitionType::NONE);
    pFormatStack_->set_hhomogeneous(true);
    pFormatStack_->set_vhomogeneous(true);
    pFormatStack_->set_hexpand(true);

    const auto PUT_TO_ENTRY= [this](const std::string &content) {
        if(insEntry_ && insEntry_->get_buffer()) insEntry_->get_buffer()->set_text(content);
    };

    for(const auto &fmt: G_kFormatOrder) {
        auto *pUi= Gtk::make_managed<InstFormatUI>(fmt);
        pUi->signalOutput_.connect(PUT_TO_ENTRY);
        formatUi_[static_cast<size_t>(fmt)]= pUi;
        const auto NAME                    = InstFormatName(fmt);
        pFormatStack_->add(*pUi, Glib::ustring(NAME.data(), NAME.size()));
    }

    auto *pEmpty= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
    pFormatStack_->add(*pEmpty, "empty");
    pFormatStack_->set_visible_child("empty");

    uiContainer_->append(*pFormatStack_);
}

void RISCVInstructionWindow::hideAllTypeUI()
{
    if(pFormatStack_) pFormatStack_->set_visible_child("empty");
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

        pushInputHistory(inputStr);
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

    const auto *pType= inst.GetTypePtr();
    auto *pCurrUi    = pType ? uiFor(pType->GetInstFormat()) : nullptr;
    if(!pCurrUi || !pFormatStack_) {
        showError("invalid input: unsupported instruction format");
        return;
    }
    pFormatStack_->set_visible_child(*pCurrUi);

    std::string buf;
    buf.reserve(128);
    auto out= std::back_inserter(buf);

    std::format_to(out, "Format:\t{}\nISA:\t{}\nManual:\t{}\n", inst.GetFormat(), inst.GetXLEN(), inst.GetManual());
    buffer->set_text(buf);
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

void RISCVInstructionWindow::setupInputHistory()
{
    insEntry_->set_icon_from_icon_name("pan-down-symbolic", Gtk::Entry::IconPosition::SECONDARY);
    insEntry_->set_icon_activatable(true, Gtk::Entry::IconPosition::SECONDARY);
    insEntry_->set_icon_tooltip_text("Input history", Gtk::Entry::IconPosition::SECONDARY);

    pHistoryPopover_= Gtk::make_managed<Gtk::Popover>();
    pHistoryPopover_->set_has_arrow(false);
    pHistoryPopover_->set_position(Gtk::PositionType::BOTTOM);
    pHistoryPopover_->set_autohide(true);
    pHistoryPopover_->set_parent(*insEntry_);
    pHistoryPopover_->add_css_class("input-history-popover");

    pHistoryScroll_= Gtk::make_managed<Gtk::ScrolledWindow>();
    pHistoryScroll_->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    pHistoryScroll_->set_propagate_natural_width(false);
    pHistoryScroll_->set_propagate_natural_height(false);

    pHistoryList_= Gtk::make_managed<Gtk::ListBox>();
    pHistoryList_->set_selection_mode(Gtk::SelectionMode::SINGLE);
    pHistoryList_->set_activate_on_single_click(true);
    pHistoryList_->add_css_class("input-history-list");
    pHistoryList_->signal_row_activated().connect([this](Gtk::ListBoxRow *pRow) {
        if(!pRow) return;
        applyHistoryAt(pRow->get_index());
        pHistoryPopover_->popdown();
        insEntry_->grab_focus();
    });
    pHistoryScroll_->set_child(*pHistoryList_);
    pHistoryPopover_->set_child(*pHistoryScroll_);

    rebuildHistoryList();

    insEntry_->signal_icon_press().connect([this](Gtk::Entry::IconPosition pos) {
        if(pos != Gtk::Entry::IconPosition::SECONDARY) return;
        popupInputHistory();
    });

    auto keyCtrl= Gtk::EventControllerKey::create();
    keyCtrl->signal_key_pressed().connect(
        [this](guint keyval, guint, Gdk::ModifierType) -> bool {
            if(keyval == GDK_KEY_Up) {
                browseHistory(+1);
                return true;
            }
            if(keyval == GDK_KEY_Down) {
                browseHistory(-1);
                return true;
            }
            return false;
        },
        false);
    insEntry_->add_controller(keyCtrl);

    insEntry_->signal_changed().connect([this] {
        if(historyBrowseIndex_ < 0
           || historyBrowseIndex_ >= inputHistory_.size()) {
            historyBrowseIndex_= -1;
            return;
        }
        if(insEntry_->get_text().raw() != inputHistory_[historyBrowseIndex_]) {
            historyBrowseIndex_= -1;
            historyDraft_.clear();
        }
    });
}

void RISCVInstructionWindow::popupInputHistory()
{
    if(inputHistory_.empty() || !pHistoryPopover_ || !insEntry_) return;

    rebuildHistoryList();

    const int ENTRY_W= std::max(insEntry_->get_width(), 200);
    const int VISIBLE= static_cast<int>(std::min(inputHistory_.size(), static_cast<size_t>(G_kHistoryVisibleRows)));
    const int LIST_H = VISIBLE * G_kHistoryRowHeight;

    pHistoryScroll_->set_size_request(ENTRY_W, LIST_H);
    pHistoryPopover_->set_size_request(ENTRY_W, LIST_H);
    pHistoryPopover_->popup();
}

void RISCVInstructionWindow::pushInputHistory(std::string_view text)
{
    if(text.empty()) return;

    const std::string ENTRY { text };
    std::erase(inputHistory_, ENTRY);
    inputHistory_.push_front(ENTRY);

    while(inputHistory_.size() > G_kMaxInputHistory) {
        inputHistory_.pop_back();
    }

    historyBrowseIndex_= -1;
    historyDraft_.clear();
    rebuildHistoryList();
}

void RISCVInstructionWindow::rebuildHistoryList()
{
    if(!pHistoryList_) return;

    while(auto *pChild= pHistoryList_->get_row_at_index(0))
        pHistoryList_->remove(*pChild);

    for(const std::string &item: inputHistory_) {
        auto *pLabel= Gtk::make_managed<Gtk::Label>(item);
        pLabel->set_halign(Gtk::Align::FILL);
        pLabel->set_xalign(0.0F);
        pLabel->set_margin_start(8);
        pLabel->set_margin_end(8);
        pLabel->set_margin_top(4);
        pLabel->set_margin_bottom(4);
        pLabel->set_ellipsize(Pango::EllipsizeMode::END);
        pLabel->set_hexpand(true);

        auto *pRow= Gtk::make_managed<Gtk::ListBoxRow>();
        pRow->set_child(*pLabel);
        pRow->set_size_request(-1, G_kHistoryRowHeight);
        pHistoryList_->append(*pRow);
    }
}

void RISCVInstructionWindow::applyHistoryAt(int index)
{
    if(index < 0 || index >= inputHistory_.size()) return;

    historyBrowseIndex_= index;
    insEntry_->set_text(inputHistory_[index]);
    insEntry_->set_position(-1);
}

void RISCVInstructionWindow::browseHistory(int delta)
{
    if(inputHistory_.empty()) return;

    if(historyBrowseIndex_ < 0) {
        historyDraft_      = insEntry_->get_text();
        historyBrowseIndex_= 0;
        if(delta < 0) {
            historyBrowseIndex_= -1;
            return;
        }
    } else {
        historyBrowseIndex_+= delta;
    }

    if(historyBrowseIndex_ < 0) {
        historyBrowseIndex_= -1;
        insEntry_->set_text(historyDraft_);
        insEntry_->set_position(-1);
        return;
    }
    if(historyBrowseIndex_ >= inputHistory_.size()) {
        historyBrowseIndex_= static_cast<int>(inputHistory_.size()) - 1;
    }
    applyHistoryAt(historyBrowseIndex_);
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
