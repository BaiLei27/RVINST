#include <algorithm>
#include <format>
#include <print>

#include "Gui/RISCVInstWindow.hh"
#include "Util/InputParse.hh"
#include "config.hpp"

namespace {

constexpr int G_MaxInputHistory   = 32;
constexpr int G_HistoryVisibleRows= 8;
constexpr int G_HistoryRowHeight  = 40;

// string_view -> Glib::ustring (safe for NUL-free ISA strings)
[[nodiscard]] Glib::ustring toGlibUstring(std::string_view sv)
{
    return { sv.data(), sv.size() };
}

} // namespace

RISCVInstWindow::RISCVInstWindow()
    : insEntry_(Gtk::make_managed<Gtk::Entry>()),
      insButtonParse_(Gtk::make_managed<Gtk::Button>("Parse Instruction")),
      pErrorLabel_(Gtk::make_managed<Gtk::Label>()),
      pSettingsBtn_(Gtk::make_managed<Gtk::MenuButton>()),
      pVersionBtn_(Gtk::make_managed<Gtk::Button>()),
      uiContainer_(Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 14)),
      history_(G_MaxInputHistory)
{
    set_title("RISC-V Instruction Encoder/Decoder");
    set_default_size(680, 720);
    add_css_class("app-window");

    uiContainer_->set_margin(12);
    uiContainer_->add_css_class("main-container");
#if defined(_WIN32)
    std::println("Windows");
    uiContainer_->add_css_class("standard"); // Windows users historically get the larger 25px-equivalent type scale.
#else
    uiContainer_->add_css_class("compact"); // Compact 16px baseline keeps the 32-bit bit diagram narrow on Linux/Wayland.
#endif

    insEntry_->set_placeholder_text("Hex (0x33), binary (0b110011), or assembly (add x0,x0,x0)");
    insEntry_->set_hexpand(true);
    insEntry_->add_css_class("input-entry");
    insEntry_->signal_activate().connect(sigc::mem_fun(*this, &RISCVInstWindow::onInsButtonParseClicked));
    setupInputHistory();

    pEntryRow_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    pEntryRow_->add_css_class("input-row");

    pVersionBtn_->set_icon_name("dialog-information-symbolic");
    pVersionBtn_->set_tooltip_text("About RVINST");
    pVersionBtn_->add_css_class("icon-button");
    pVersionBtn_->signal_clicked().connect([this] { showVersionWindow(); });
    pEntryRow_->append(*pVersionBtn_);
    pEntryRow_->append(*insEntry_);

    pFileBtn_= Gtk::make_managed<Gtk::Button>();
    pFileBtn_->set_icon_name("document-open-symbolic");
    pFileBtn_->set_tooltip_text("Open input file");
    pFileBtn_->add_css_class("icon-button");
    pFileBtn_->signal_clicked().connect(sigc::mem_fun(*this, &RISCVInstWindow::onOpenFileClicked));
    pEntryRow_->append(*pFileBtn_);

    pSettingsBtn_->set_icon_name("emblem-system-symbolic");
    pSettingsBtn_->set_tooltip_text("ABI / ISA settings");
    pSettingsBtn_->set_always_show_arrow(false);
    pSettingsBtn_->add_css_class("icon-button");
    setupSettingsPopover();
    pEntryRow_->append(*pSettingsBtn_);

    insButtonParse_->add_css_class("parse-button");
    insButtonParse_->set_halign(Gtk::Align::CENTER);
    insButtonParse_->signal_clicked().connect(sigc::mem_fun(*this, &RISCVInstWindow::onInsButtonParseClicked));

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

    signal_map().connect([this] {
        if(insEntry_) insEntry_->grab_focus();
    });
}

RISCVInstWindow::~RISCVInstWindow()
{
    if(pHistoryPopover_) pHistoryPopover_->unparent();
    if(hasSaveHist_) std::ignore= fileMgr_.SaveHistory(history_);
}

void RISCVInstWindow::initInstFormatUI()
{
    pFormatStack_= Gtk::make_managed<Gtk::Stack>();
    pFormatStack_->set_transition_type(Gtk::StackTransitionType::NONE);
    pFormatStack_->set_hhomogeneous(true);
    pFormatStack_->set_vhomogeneous(true);
    pFormatStack_->set_hexpand(true);
    pFormatStack_->set_vexpand(true);
    pFormatStack_->add_css_class("format-stack");

    // Single panel doubles as the pre-parse default view (was a disabled R-Type
    // InstFormatUI placeholder). Keep it visible at startup; only grey it out
    // until the first successful parse.
    pFormatUi_= Gtk::make_managed<InstFormatUI>(InstFormat::R);
    pFormatUi_->set_sensitive(false);
    pFormatUi_->add_css_class("format-placeholder");
    pFormatUi_->signalOutput_.connect([this](const std::string &content) {
        if(insEntry_ && insEntry_->get_buffer()) {
            insEntry_->get_buffer()->set_text(content);
            insEntry_->grab_focus();
            insEntry_->set_position(-1);
        }
    });
    pFormatStack_->add(*pFormatUi_, "result");
    pFormatStack_->set_visible_child(*pFormatUi_);
}

void RISCVInstWindow::hideFormatUI()
{
    // Match the old "empty" placeholder: keep the diagram visible, disable input.
    if(pFormatUi_) {
        pFormatUi_->set_sensitive(false);
        pFormatUi_->add_css_class("format-placeholder");
    }
}

void RISCVInstWindow::onInsButtonParseClicked()
{
    const auto TEXT= insEntry_->get_text();
    if(TEXT.empty()) {
        showError("invalid input: empty");
        return;
    }

    std::string inputStr(TEXT);
    inst_.reset();

    const auto PARSED= util::ClassifyInstInput(inputStr);
    if(!PARSED) {
        showError(std::format("invalid input: {}", PARSED.error().Msg()));
        insEntry_->grab_focus();
        return;
    }

    if(PARSED->kind_ == util::InputKind::ASSEMBLY) {
        inst_= std::make_unique<Instruction>(inputStr, hasSetABI_);
    } else {
        inst_= std::make_unique<Instruction>(PARSED->word_, hasSetABI_);
    }

    if(const auto DECODED= inst_->Decode(); !DECODED) {
        showError(std::format("invalid input: {}", DECODED.error().Msg()));
        insEntry_->grab_focus();
        return;
    }

    pushInputHistory(inputStr);
    showInsResult(*inst_);
    insEntry_->grab_focus();
}

void RISCVInstWindow::showInsResult(Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType || !pFormatUi_ || !pFormatStack_) {
        showError("invalid input: unsupported instruction format");
        return;
    }

    if(pType->GetInstFormat() == InstFormat::UNKNOWN) {
        showError("invalid input: unsupported instruction format");
        return;
    }

    if(pErrorCard_) pErrorCard_->set_visible(false);
    pFormatUi_->remove_css_class("format-placeholder");
    pFormatUi_->set_sensitive(true);
    pFormatStack_->set_visible_child(*pFormatUi_);
    pFormatUi_->SetHexImm(hasHexImm_);
    pFormatUi_->UpdateDisplay(inst);
    fitWindowToContent(false);
}

void RISCVInstWindow::fitWindowToContent(bool allowShrink)
{
    // Wait one idle so CSS/font changes settle before measuring the bit diagram.
    Glib::signal_idle().connect_once([this, allowShrink] {
        if(!pFormatUi_ || !pFormatUi_->FitToContent()) return;

        const int PAD   = uiContainer_ ? (uiContainer_->get_margin_start() + uiContainer_->get_margin_end()) : 24;
        const int NEED_W= std::max(pFormatUi_->MeasureWidth() + PAD + 28, 560);
        const int CUR_H = std::max(get_height(), 720);

        if(allowShrink) {
            // Clear the previous min-width so GTK can shrink after compact font.
            // Re-assert the min-width on a later idle: set_default_size is a request,
            // and a same-frame set_size_request would win and block the shrink.
            set_size_request(-1, -1);
            set_default_size(NEED_W, CUR_H);
            Glib::signal_idle().connect_once([this, needW= NEED_W] {
                set_size_request(needW, -1);
                if(pFormatUi_) pFormatUi_->RestoreHexPand();
            });
            return;
        }

        set_size_request(NEED_W, -1);
        const int CUR_W= get_width();
        if(NEED_W > CUR_W || get_height() < 720) {
            set_default_size(std::max(NEED_W, CUR_W), CUR_H);
        }
        pFormatUi_->RestoreHexPand();
    });
}

void RISCVInstWindow::showError(const std::string &message)
{
    hideFormatUI();
    inst_.reset();
    if(pErrorLabel_) pErrorLabel_->set_text("[Error]: " + message);
    if(pErrorCard_) pErrorCard_->set_visible(true);
    // Safe under WIN32_EXECUTABLE after Main::InitGuiStdio (NUL or parent console).
    // std::println(stderr, "Error: {}", message);
    // std::println("Error: {}", message);
}

void RISCVInstWindow::refreshAssemblyDisplay()
{
    if(!inst_) return;

    const auto VAL= static_cast<uint32_t>(*inst_);
    inst_         = std::make_unique<Instruction>(VAL, hasSetABI_);
    if(const auto DECODED= inst_->Decode(); DECODED) {
        showInsResult(*inst_);
    } else {
        showError(std::format("invalid input: {}", DECODED.error().Msg()));
    }
}

void RISCVInstWindow::setupInputHistory()
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
        if(!history_.IsBrowsing()) return;

        if(insEntry_->get_text().raw() != history_.At(history_.CursorIndex())) {
            history_.CancelBrowse();
        }
    });
}

void RISCVInstWindow::popupInputHistory()
{
    if(history_.IsEmpty() || !pHistoryPopover_ || !insEntry_) return;

    rebuildHistoryList();

    const int ENTRY_W= std::max({ insEntry_->get_width(), insEntry_->get_allocated_width(), 260 });
    const int VISIBLE= static_cast<int>(std::min(history_.Size(), static_cast<size_t>(G_HistoryVisibleRows)));
    const int LIST_H = VISIBLE * G_HistoryRowHeight;

    bool needScroll= history_.Size() > static_cast<size_t>(G_HistoryVisibleRows);

    pHistoryScroll_->set_propagate_natural_width(false);
    pHistoryScroll_->set_propagate_natural_height(false);
    pHistoryScroll_->set_policy(Gtk::PolicyType::NEVER, needScroll ? Gtk::PolicyType::AUTOMATIC : Gtk::PolicyType::NEVER);

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

void RISCVInstWindow::pushInputHistory(std::string_view text)
{
    history_.Push(text);
    rebuildHistoryList();
}

void RISCVInstWindow::rebuildHistoryList()
{
    if(!pHistoryList_) return;

    int i= 0;
    for(const auto &item: history_.GetEntries()) {
        auto *pRow= pHistoryList_->get_row_at_index(i);
        if(!pRow) {
            pRow= Gtk::make_managed<Gtk::ListBoxRow>();
            pRow->set_size_request(-1, G_HistoryRowHeight);
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

    while(auto *pRow= pHistoryList_->get_row_at_index(i)) {
        pHistoryList_->remove(*pRow);
    }
}

void RISCVInstWindow::applyHistoryAt(size_t index)
{
    history_.JumpTo(index);
    if(!history_.IsBrowsing()) return;
    insEntry_->set_text(toGlibUstring(history_.At(history_.CursorIndex())));
    insEntry_->set_position(-1);
}

void RISCVInstWindow::browseHistory(int delta)
{
    if(history_.IsEmpty()) return;

    if(!history_.IsBrowsing()) {
        history_.SetDraft(insEntry_->get_text().raw());
    }
    const auto STEP= history_.Step(delta);
    if(!STEP) return;

    insEntry_->set_text(toGlibUstring(*STEP));
    insEntry_->set_position(-1);
}

void RISCVInstWindow::onOpenFileClicked()
{
    // Keep the dialog alive for the async callback: a plain local would be
    // destroyed when this handler returns. Reuse the member if one is open.
    if(!pFileDialog_) {
        pFileDialog_= Gtk::FileDialog::create();
        pFileDialog_->set_title("Open input file");

        auto pFilter= Gtk::FileFilter::create();
        pFilter->set_name("Text and data files");
        pFilter->add_pattern("*.txt");
        pFilter->add_pattern("*.log");
        pFilter->add_pattern("*.data");
        pFilter->add_pattern("*");
        auto pFilters= Gio::ListStore<Gtk::FileFilter>::create();
        pFilters->append(pFilter);
        pFileDialog_->set_filters(pFilters);
    }

    pFileDialog_->open(*this, [this](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file= pFileDialog_->open_finish(result);
            if(file) processFileLines(file->get_path());
        } catch(const Gtk::DialogError &err) {
            if(Gtk::DialogError::Code::DISMISSED != err.code()) {
                showError(std::format("file open failed: {}", err.what()));
            }
        } catch(const Glib::Error &err) {
            showError(std::format("file open failed: {}", err.what()));
        }
        pFileDialog_.reset(); // release after the callback runs
    });
}

void RISCVInstWindow::processFileLines(std::string_view path)
{
    Instruction *pLastInst= nullptr;
    auto streamed         = fileMgr_.ForEachLine(path, [this, &pLastInst](std::string_view raw) {
        const auto TRIMMED= util::TRIM_ASCII_WS(raw);
        if(TRIMMED.empty()) return;

        auto parsed= util::ClassifyInstInput(TRIMMED);
        if(!parsed) {
            showError(std::format("invalid input: {}", parsed.error().Msg()));
            return;
        }

        std::unique_ptr<Instruction> inst;
        if(util::InputKind::ASSEMBLY == parsed->kind_) {
            inst= std::make_unique<Instruction>(TRIMMED, hasSetABI_);
        } else {
            inst= std::make_unique<Instruction>(parsed->word_, hasSetABI_);
        }
        if(!inst->Decode()) {
            return; // skip invalid lines silently in batch mode
        }

        pushInputHistory(TRIMMED); // merge into shared history (browsable + :hist)
        pLastInst= inst.get();
        inst_    = std::move(inst);
    });
    if(!streamed) {
        showError(std::format("file read failed: {}", streamed.error().Msg()));
        return;
    }

    if(pLastInst) {
        showInsResult(*pLastInst);
        insEntry_->grab_focus();
    } else {
        showError("no valid instructions found in file");
    }
}

void RISCVInstWindow::setupSettingsPopover()
{
    // MenuButton owns the popover; avoids Button+set_parent press-state bleed
    // into the settings icon when clicking ABI/ISA controls inside the popover.
    auto *pPopover= Gtk::make_managed<Gtk::Popover>();
    pPopover->set_has_arrow(true);
    pPopover->add_css_class("settings-popover");

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
        refreshAssemblyDisplay();
    });
    pAbiRow->append(*pAbiSwitch_);
    pPopoverBox->append(*pAbiRow);

    auto *pHexRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pHexRow->set_halign(Gtk::Align::START);
    pHexRow->add_css_class("settings-row");

    auto *pHexLabel= Gtk::make_managed<Gtk::Label>("Hex imm");
    pHexLabel->set_halign(Gtk::Align::START);
    pHexLabel->set_hexpand(false);
    pHexLabel->add_css_class("settings-label");
    pHexRow->append(*pHexLabel);

    pHexImmSwitch_= Gtk::make_managed<Gtk::Switch>();
    pHexImmSwitch_->set_active(hasHexImm_);
    pHexImmSwitch_->set_halign(Gtk::Align::START);
    pHexImmSwitch_->set_valign(Gtk::Align::CENTER);
    pHexImmSwitch_->add_css_class("hex-imm-switch");
    pHexImmSwitch_->property_active().signal_changed().connect([this] {
        hasHexImm_= pHexImmSwitch_->get_active();
        if(!inst_ || !pFormatUi_) return;
        pFormatUi_->SetHexImm(hasHexImm_);
        pFormatUi_->UpdateDisplay(*inst_);
    });
    pHexRow->append(*pHexImmSwitch_);
    pPopoverBox->append(*pHexRow);

    auto *pSaveHistRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pSaveHistRow->set_halign(Gtk::Align::START);
    pSaveHistRow->add_css_class("settings-row");

    auto *pSaveHistLabel= Gtk::make_managed<Gtk::Label>("Save history");
    pSaveHistLabel->set_halign(Gtk::Align::START);
    pSaveHistLabel->set_hexpand(false);
    pSaveHistLabel->add_css_class("settings-label");
    pSaveHistRow->append(*pSaveHistLabel);

    pSaveHistSwitch_= Gtk::make_managed<Gtk::Switch>();
    pSaveHistSwitch_->set_active(hasSaveHist_);
    pSaveHistSwitch_->set_halign(Gtk::Align::START);
    pSaveHistSwitch_->set_valign(Gtk::Align::CENTER);
    pSaveHistSwitch_->add_css_class("save-hist-switch");
    pSaveHistSwitch_->property_active().signal_changed().connect([this] {
        hasSaveHist_= pSaveHistSwitch_->get_active();
        // Enabling loads the previous session's history; disabling only stops
        // future writes (the in-memory history stays browsable).
        if(hasSaveHist_) std::ignore= fileMgr_.LoadHistory(history_);
    });
    pSaveHistRow->append(*pSaveHistSwitch_);
    pPopoverBox->append(*pSaveHistRow);

    auto *pFontRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pFontRow->set_halign(Gtk::Align::START);
    pFontRow->add_css_class("settings-row");

    auto *pFontLabel= Gtk::make_managed<Gtk::Label>("Compact");
    pFontLabel->set_halign(Gtk::Align::START);
    pFontLabel->set_hexpand(false);
    pFontLabel->add_css_class("settings-label");
    pFontRow->append(*pFontLabel);

    pFontSizeSwitch_= Gtk::make_managed<Gtk::Switch>();
    pFontSizeSwitch_->set_active(uiContainer_->has_css_class("compact"));
    pFontSizeSwitch_->set_halign(Gtk::Align::START);
    pFontSizeSwitch_->set_valign(Gtk::Align::CENTER);
    pFontSizeSwitch_->add_css_class("font-size-switch");
    pFontSizeSwitch_->property_active().signal_changed().connect([this] {
        const bool COMPACT= pFontSizeSwitch_->get_active();
        if(COMPACT) {
            uiContainer_->remove_css_class("standard");
            uiContainer_->add_css_class("compact");
        } else {
            uiContainer_->remove_css_class("compact");
            uiContainer_->add_css_class("standard");
        }
        // Compact -> allow shrink; Standard -> grow only.
        fitWindowToContent(COMPACT);
    });
    pFontRow->append(*pFontSizeSwitch_);
    pPopoverBox->append(*pFontRow);

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
    pIsaDrop_->set_selected(0); // AUTO
    pIsaDrop_->set_hexpand(false);
    pIsaDrop_->set_halign(Gtk::Align::START);
    pIsaDrop_->set_size_request(86, -1);
    pIsaDrop_->add_css_class("isa-dropdown");
    pIsaRow->append(*pIsaDrop_);
    pPopoverBox->append(*pIsaRow);

    pPopover->set_child(*pPopoverBox);
    pSettingsBtn_->set_popover(*pPopover);
}

void RISCVInstWindow::showVersionWindow()
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

            auto *pName= Gtk::make_managed<Gtk::Label>(toGlibUstring(name));
            pName->set_halign(Gtk::Align::START);
            pName->set_valign(Gtk::Align::CENTER);
            pName->set_xalign(0.0F);
            pName->set_width_chars(10);
            pName->add_css_class("version-info-name");

            auto *pValue= Gtk::make_managed<Gtk::Label>(toGlibUstring(value));
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

void RISCVInstWindow::loadCSSFromFile()
{
    auto cssProvider= Gtk::CssProvider::create();
    cssProvider->load_from_file(Gio::File::create_for_path(CSS_FILE_PATH));
    if(auto display= Gdk::Display::get_default()) {
        Gtk::StyleContext::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}
