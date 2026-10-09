#include <algorithm>
#include <array>
#include <format>
#include <print>

#include "Gui/BinaryFieldWidget.hh"
#include "Gui/RISCVInstructionWindow.hh"
#include "Util/InputParse.hpp"
#include "config.hpp"

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
constexpr int G_kHistoryRowHeight  = 40;

} // namespace

RISCVInstructionWindow::RISCVInstructionWindow()
    : insEntry_(Gtk::make_managed<Gtk::Entry>()),
      insButtonParse_(Gtk::make_managed<Gtk::Button>("Parse Instruction")),
      pErrorLabel_(Gtk::make_managed<Gtk::Label>()),
      pSettingsBtn_(Gtk::make_managed<Gtk::Button>()),
      pVersionBtn_(Gtk::make_managed<Gtk::Button>()),
      uiContainer_(Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 14))
{
    set_title("RISC-V Instruction Encoder/Decoder");
    set_default_size(680, 720);
    add_css_class("app-window");

    uiContainer_->set_margin(12);
    uiContainer_->add_css_class("main-container");

    insEntry_->set_placeholder_text("Hex (0x33), binary (0b110011), or assembly (add x0,x0,x0)");
    insEntry_->set_hexpand(true);
    insEntry_->add_css_class("input-entry");
    insEntry_->signal_activate().connect(sigc::mem_fun(*this, &RISCVInstructionWindow::onInsButtonParseClicked));
    setupInputHistory();

    pEntryRow_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    pEntryRow_->add_css_class("input-row");

    pVersionBtn_->set_icon_name("dialog-information-symbolic");
    pVersionBtn_->set_tooltip_text("About RVINST");
    pVersionBtn_->add_css_class("icon-button");
    pVersionBtn_->signal_clicked().connect([this] { showVersionWindow(); });
    pEntryRow_->append(*pVersionBtn_);
    pEntryRow_->append(*insEntry_);

    pSettingsBtn_->set_icon_name("emblem-system-symbolic");
    pSettingsBtn_->set_tooltip_text("ABI / ISA settings");
    pSettingsBtn_->add_css_class("icon-button");
    setupSettingsPopover();
    pEntryRow_->append(*pSettingsBtn_);

    insButtonParse_->add_css_class("parse-button");
    insButtonParse_->set_halign(Gtk::Align::CENTER);
    insButtonParse_->signal_clicked().connect(sigc::mem_fun(*this, &RISCVInstructionWindow::onInsButtonParseClicked));

    pErrorCard_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
    pErrorCard_->add_css_class("error-card");
    pErrorCard_->set_visible(false);

    pErrorLabel_->set_halign(Gtk::Align::START);
    pErrorLabel_->set_wrap(true);
    pErrorLabel_->set_xalign(0.0F);
    pErrorLabel_->add_css_class("error-result");
    pErrorCard_->append(*pErrorLabel_);

    loadCSSFromFile();
    initInstFormatUI();

    uiContainer_->append(*pEntryRow_);
    uiContainer_->append(*insButtonParse_);
    uiContainer_->append(*pErrorCard_);
    uiContainer_->append(*pFormatStack_);
    set_child(*uiContainer_);
}

RISCVInstructionWindow::~RISCVInstructionWindow()
{
    if(pHistoryPopover_) pHistoryPopover_->unparent();
    if(pSettingsPopover_) pSettingsPopover_->unparent();
}

void RISCVInstructionWindow::initInstFormatUI()
{
    pFormatStack_= Gtk::make_managed<Gtk::Stack>();
    pFormatStack_->set_transition_type(Gtk::StackTransitionType::NONE);
    pFormatStack_->set_hhomogeneous(true);
    pFormatStack_->set_vhomogeneous(true);
    pFormatStack_->set_hexpand(true);
    pFormatStack_->set_vexpand(true);
    pFormatStack_->add_css_class("format-stack");

    const auto PUT_TO_ENTRY= [this](const std::string &content) {
        if(insEntry_ && insEntry_->get_buffer()) {
            insEntry_->get_buffer()->set_text(content);
            insEntry_->grab_focus();
            insEntry_->set_position(-1);
        }
    };

    for(const auto &fmt: G_kFormatOrder) {
        auto *pUi= Gtk::make_managed<InstFormatUI>(fmt);
        pUi->signalOutput_.connect(PUT_TO_ENTRY);
        formatUi_[static_cast<size_t>(fmt)]= pUi;
        const auto NAME                    = InstFormatName(fmt);
        pFormatStack_->add(*pUi, Glib::ustring(NAME.data(), NAME.size()));
    }

    // Keep same height as format panels before the first parse.
    auto *pPlaceholder= Gtk::make_managed<InstFormatUI>(InstFormat::R);
    pPlaceholder->set_sensitive(false);
    pPlaceholder->add_css_class("format-placeholder");
    pFormatStack_->add(*pPlaceholder, "empty");
    pFormatStack_->set_visible_child("empty");
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
    const auto *pType= inst.GetTypePtr();
    InstFormatUI *pCurrUi {};

    if(pType) {
        const int IDX= static_cast<int>(pType->GetInstFormat());
        if(IDX >= 0 && IDX < static_cast<int>(formatUi_.size())) {
            pCurrUi= formatUi_[static_cast<size_t>(IDX)];
        }
    }

    if(!pCurrUi || !pFormatStack_) {
        showError("invalid input: unsupported instruction format");
        return;
    }

    if(pErrorCard_) pErrorCard_->set_visible(false);
    pFormatStack_->set_visible_child(*pCurrUi);
    pCurrUi->UpdateDisplay(inst);

    Glib::signal_idle().connect_once([this, pCurrUi] {
        if(!pCurrUi || !pCurrUi->pBitsRow_) return;

        // Temporarily unexpand fields to measure intrinsic bit-diagram width.
        for(auto &pField: pCurrUi->binaryOwned_) {
            if(pField && pField->GetRoot()) pField->GetRoot()->set_hexpand(false);
        }
        pCurrUi->pBitsRow_->set_hexpand(false);

        int minW= 0, natW= 0, minBase= -1, natBase= -1;
        pCurrUi->pBitsRow_->measure(Gtk::Orientation::HORIZONTAL, -1, minW, natW, minBase, natBase);

        const int PAD   = uiContainer_ ? (uiContainer_->get_margin_start() + uiContainer_->get_margin_end()) : 24;
        const int WIDTH = std::max(natW + PAD + 28, 560);
        const int HEIGHT= std::max(get_height(), 720);

        set_size_request(WIDTH, -1);
        set_default_size(WIDTH, HEIGHT);

        for(auto &pField: pCurrUi->binaryOwned_) {
            if(pField && pField->GetRoot()) pField->GetRoot()->set_hexpand(true);
        }
        pCurrUi->pBitsRow_->set_hexpand(true);
        pCurrUi->pBitsRow_->queue_resize();
    });
}

void RISCVInstructionWindow::showError(const std::string &message)
{
    hideAllTypeUI();
    inst_.reset();
    if(pErrorLabel_) pErrorLabel_->set_text("Error: " + message);
    if(pErrorCard_) pErrorCard_->set_visible(true);
    std::println(stderr, "Error: {}", message);
}

void RISCVInstructionWindow::refreshAssemblyForAbiChange()
{
    if(!inst_) return;

    const auto VAL= static_cast<uint32_t>(*inst_);
    inst_         = std::make_unique<Instruction>(VAL, hasSetABI_);
    if(inst_->Decode()) {
        showInsResult(*inst_);
    } else {
        showError("invalid input: failed to decode after ABI change");
    }
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
    pHistoryScroll_->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::NEVER);
    pHistoryScroll_->set_propagate_natural_width(true);
    pHistoryScroll_->set_propagate_natural_height(true);

    pHistoryList_= Gtk::make_managed<Gtk::ListBox>();
    pHistoryList_->set_selection_mode(Gtk::SelectionMode::SINGLE);
    pHistoryList_->set_activate_on_single_click(true);
    pHistoryList_->add_css_class("input-history-list");
    pHistoryList_->signal_row_activated().connect([this](Gtk::ListBoxRow *pRow) {
        if(!pRow) return;
        applyHistoryAt(static_cast<size_t>(pRow->get_index()));
        pHistoryPopover_->popdown();
        insEntry_->grab_focus();
    });

    pHistoryScroll_->set_child(*pHistoryList_);
    pHistoryPopover_->set_child(*pHistoryScroll_);

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
        if(!historyBrowseIndex_ || *historyBrowseIndex_ >= inputHistory_.size()) {
            historyBrowseIndex_.reset();
            return;
        }
        if(insEntry_->get_text().raw() != inputHistory_[*historyBrowseIndex_]) {
            historyBrowseIndex_.reset();
            historyDraft_.clear();
        }
    });
}

void RISCVInstructionWindow::popupInputHistory()
{
    if(inputHistory_.empty() || !pHistoryPopover_ || !insEntry_) return;

    if(historyDirty_) {
        rebuildHistoryList();
        historyDirty_= false;
    }

    const int ENTRY_W= std::max(
        { insEntry_->get_width(), insEntry_->get_allocated_width(), 260 });
    const int VISIBLE=
        static_cast<int>(std::min(inputHistory_.size(), static_cast<size_t>(G_kHistoryVisibleRows)));
    const int LIST_H      = VISIBLE * G_kHistoryRowHeight;
    const bool NEED_SCROLL= inputHistory_.size() > static_cast<size_t>(G_kHistoryVisibleRows);

    pHistoryScroll_->set_propagate_natural_width(false);
    pHistoryScroll_->set_propagate_natural_height(false);
    pHistoryScroll_->set_policy(Gtk::PolicyType::NEVER,
                                NEED_SCROLL ? Gtk::PolicyType::AUTOMATIC : Gtk::PolicyType::NEVER);

    // Reset first: setting min while a smaller max remains triggers Gtk-CRITICAL.
    pHistoryScroll_->set_min_content_width(-1);
    pHistoryScroll_->set_max_content_width(-1);
    pHistoryScroll_->set_min_content_height(-1);
    pHistoryScroll_->set_max_content_height(-1);

    pHistoryScroll_->set_max_content_width(ENTRY_W);
    pHistoryScroll_->set_min_content_width(ENTRY_W);
    pHistoryScroll_->set_max_content_height(LIST_H);
    pHistoryScroll_->set_min_content_height(LIST_H);
    pHistoryScroll_->set_size_request(ENTRY_W, LIST_H);
    pHistoryPopover_->set_size_request(ENTRY_W, -1);
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

    historyBrowseIndex_.reset();
    historyDraft_.clear();

    if(pHistoryPopover_ && pHistoryPopover_->get_visible()) {
        rebuildHistoryList();
    } else {
        historyDirty_= true;
    }
}

void RISCVInstructionWindow::rebuildHistoryList()
{
    if(!pHistoryList_) return;

    size_t i= 0;
    for(const auto &item: inputHistory_) {
        auto *pRow= pHistoryList_->get_row_at_index(static_cast<int>(i));
        if(!pRow) {
            pRow= Gtk::make_managed<Gtk::ListBoxRow>();
            pRow->set_size_request(-1, G_kHistoryRowHeight);
            pRow->add_css_class("input-history-row");

            auto *pLabel= Gtk::make_managed<Gtk::Label>();
            pLabel->set_halign(Gtk::Align::FILL);
            pLabel->set_xalign(0.0F);
            pLabel->set_margin_start(10);
            pLabel->set_margin_end(10);
            pLabel->set_margin_top(4);
            pLabel->set_margin_bottom(4);
            pLabel->set_ellipsize(Pango::EllipsizeMode::END);
            pLabel->set_hexpand(true);
            pLabel->add_css_class("input-history-label");
            pRow->set_child(*pLabel);
            pHistoryList_->append(*pRow);
        }
        if(auto *pLabel= dynamic_cast<Gtk::Label *>(pRow->get_child())) {
            pLabel->set_text(item);
        }
        ++i;
    }

    while(auto *pRow= pHistoryList_->get_row_at_index(static_cast<int>(i))) {
        pHistoryList_->remove(*pRow);
    }
}

void RISCVInstructionWindow::applyHistoryAt(size_t index)
{
    if(index >= inputHistory_.size()) return;
    historyBrowseIndex_= index;
    insEntry_->set_text(inputHistory_[index]);
    insEntry_->set_position(-1);
}

void RISCVInstructionWindow::browseHistory(int delta)
{
    if(inputHistory_.empty()) return;

    if(!historyBrowseIndex_) {
        historyDraft_= insEntry_->get_text();
        if(delta < 0) return;
        historyBrowseIndex_= 0;
    } else {
        const size_t CUR= *historyBrowseIndex_;
        if(delta < 0) {
            if(CUR == 0) {
                historyBrowseIndex_.reset();
                insEntry_->set_text(historyDraft_);
                insEntry_->set_position(-1);
                return;
            }
            historyBrowseIndex_= CUR - 1;
        } else {
            historyBrowseIndex_= CUR + static_cast<size_t>(delta);
        }
    }

    if(*historyBrowseIndex_ >= inputHistory_.size()) {
        historyBrowseIndex_= inputHistory_.size() - 1;
    }
    applyHistoryAt(*historyBrowseIndex_);
}

void RISCVInstructionWindow::setupSettingsPopover()
{
    pSettingsPopover_= Gtk::make_managed<Gtk::Popover>();
    pSettingsPopover_->set_has_arrow(true);
    pSettingsPopover_->set_parent(*pSettingsBtn_);
    pSettingsPopover_->add_css_class("settings-popover");

    auto *pPopoverBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    pPopoverBox->set_margin(14);
    pPopoverBox->add_css_class("settings-content");

    auto *pAbiRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pAbiRow->set_halign(Gtk::Align::START);
    pAbiRow->add_css_class("settings-row");

    auto *pAbiLabel= Gtk::make_managed<Gtk::Label>("ABI");
    pAbiLabel->set_halign(Gtk::Align::START);
    pAbiLabel->set_hexpand(false);
    pAbiLabel->add_css_class("settings-label");
    pAbiRow->append(*pAbiLabel);

    pAbiSwitch_= Gtk::make_managed<Gtk::Switch>();
    pAbiSwitch_->set_active(hasSetABI_);
    pAbiSwitch_->set_halign(Gtk::Align::START);
    pAbiSwitch_->set_valign(Gtk::Align::CENTER);
    pAbiSwitch_->add_css_class("abi-switch");
    pAbiSwitch_->property_active().signal_changed().connect([this] {
        hasSetABI_= pAbiSwitch_->get_active();
        refreshAssemblyForAbiChange();
    });
    pAbiRow->append(*pAbiSwitch_);
    pPopoverBox->append(*pAbiRow);

    auto *pIsaRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pIsaRow->set_halign(Gtk::Align::START);
    pIsaRow->add_css_class("settings-row");

    auto *pIsaLabel= Gtk::make_managed<Gtk::Label>("ISA");
    pIsaLabel->set_halign(Gtk::Align::START);
    pIsaLabel->set_hexpand(false);
    pIsaLabel->set_size_request(36, -1);
    pIsaLabel->add_css_class("settings-label");
    pIsaRow->append(*pIsaLabel);

    auto isaList= Gtk::StringList::create({ "AUTO", "RV32I", "RV64I", "RV128I" });
    pIsaDrop_   = Gtk::make_managed<Gtk::DropDown>(isaList);
    pIsaDrop_->set_selected(static_cast<guint>(selectedIsaIndex_));
    pIsaDrop_->set_hexpand(false);
    pIsaDrop_->set_halign(Gtk::Align::START);
    pIsaDrop_->set_size_request(86, -1);
    pIsaDrop_->add_css_class("isa-dropdown");
    pIsaDrop_->property_selected().signal_changed().connect([this] {
        if(!pIsaDrop_) return;
        selectedIsaIndex_= static_cast<int>(pIsaDrop_->get_selected());
    });
    pIsaRow->append(*pIsaDrop_);
    pPopoverBox->append(*pIsaRow);

    pSettingsPopover_->set_child(*pPopoverBox);
    pSettingsBtn_->signal_clicked().connect([this] { pSettingsPopover_->popup(); });
}

void RISCVInstructionWindow::showVersionWindow()
{
    if(!pVersionWindow_) {
        pVersionWindow_= std::make_unique<Gtk::Window>();
        pVersionWindow_->set_title("About RVINST");
        pVersionWindow_->set_transient_for(*this);
        pVersionWindow_->set_modal(true);
        pVersionWindow_->set_resizable(false);
        pVersionWindow_->set_hide_on_close(true);
        pVersionWindow_->add_css_class("about-window");

        auto *pBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
        pBox->set_margin(24);
        pBox->set_halign(Gtk::Align::FILL);
        pBox->add_css_class("about-container");

        auto *pTitle= Gtk::make_managed<Gtk::Label>("RVINST");
        pTitle->add_css_class("version-title");
        pTitle->set_halign(Gtk::Align::START);
        pTitle->set_xalign(0.0F);
        pTitle->set_justify(Gtk::Justification::LEFT);
        pBox->append(*pTitle);

        auto *pInfoCard= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
        pInfoCard->set_halign(Gtk::Align::FILL);
        pInfoCard->add_css_class("version-info-card");

        auto appendInfoRow= [pInfoCard](std::string_view name, std::string_view value, bool link= false) {
            auto *pRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
            pRow->set_halign(Gtk::Align::FILL);
            pRow->add_css_class("version-info-row");

            auto *pName= Gtk::make_managed<Gtk::Label>(
                Glib::ustring(name.data(), name.size()));
            pName->set_halign(Gtk::Align::START);
            pName->set_valign(Gtk::Align::CENTER);
            pName->set_xalign(0.0F);
            pName->set_width_chars(10);
            pName->add_css_class("version-info-name");

            auto *pValue= Gtk::make_managed<Gtk::Label>(
                Glib::ustring(value.data(), value.size()));
            pValue->set_halign(Gtk::Align::START);
            pValue->set_valign(Gtk::Align::CENTER);
            pValue->set_hexpand(true);
            pValue->set_selectable(true);
            pValue->set_can_focus(false);
            pValue->set_xalign(0.0F);
            pValue->add_css_class("version-info-value");
            if(link) pValue->add_css_class("version-info-link");

            pRow->append(*pName);
            pRow->append(*pValue);
            pInfoCard->append(*pRow);
        };

        appendInfoRow("Version:", util::info::G_PROJECT_VERSION);
        appendInfoRow("Commit:", util::info::G_COMMIT_HASH);
        appendInfoRow("URL:", util::info::G_HOMEPAGE_URL, true);
        appendInfoRow("Copyright:", util::info::G_COPYRIGHT);
        appendInfoRow("GTKMM:", std::format("{}.{}.{}", GTKMM_MAJOR_VERSION, GTKMM_MINOR_VERSION, GTKMM_MICRO_VERSION));
        appendInfoRow("GTK:", std::format("{}.{}.{}", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION));
        appendInfoRow("GLib:", std::format("{}.{}.{}", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION));

        pBox->append(*pInfoCard);

        auto *pCloseBtn= Gtk::make_managed<Gtk::Button>("Close");
        pCloseBtn->set_halign(Gtk::Align::END);
        pCloseBtn->add_css_class("about-close-button");
        pCloseBtn->signal_clicked().connect([this] { pVersionWindow_->hide(); });
        pBox->append(*pCloseBtn);

        pVersionWindow_->set_child(*pBox);
    }

    pVersionWindow_->present();
}

void RISCVInstructionWindow::loadCSSFromFile()
{
    try {
        auto cssProvider= Gtk::CssProvider::create();
        cssProvider->load_from_file(Gio::File::create_for_path(CSS_FILE_PATH));
        if(auto display= Gdk::Display::get_default()) {
            Gtk::StyleContext::add_provider_for_display(
                display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    } catch(const Glib::Error &ex) {
        std::println(stderr, "CSS error: {}", ex.what());
    }
}
