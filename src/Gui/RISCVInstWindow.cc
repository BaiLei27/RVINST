#include <algorithm>
#include <format>

#include "Gui/GlibString.hh"
#include "Gui/RISCVInstWindow.hh"
#include "Util/InputParse.hh"

namespace {

constexpr int G_MaxInputHistory   = 32;
constexpr int G_HistoryVisibleRows= 8;
constexpr int G_HistoryRowHeight  = 40;

} // namespace

RISCVInstWindow::RISCVInstWindow()
    : insEntry_(Gtk::make_managed<Gtk::Entry>()),
      insButtonParse_(Gtk::make_managed<Gtk::Button>("Parse Instruction")),
      pErrorLabel_(Gtk::make_managed<Gtk::Label>()),
      uiContainer_(Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 14)),
      history_(G_MaxInputHistory)
{
    set_title("RISC-V Instruction Encoder/Decoder");
    set_default_size(680, 720);
    add_css_class("app-window");

    uiContainer_->set_margin(12);
    uiContainer_->add_css_class("main-container");
#ifdef _WIN32
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

    // About button (right of entry) — window owned by AboutWindow.
    pAbout_          = std::make_unique<AboutWindow>(*this);
    auto *pVersionBtn= Gtk::make_managed<Gtk::Button>();
    pVersionBtn->set_icon_name("dialog-information-symbolic");
    pVersionBtn->set_tooltip_text("About RVINST");
    pVersionBtn->add_css_class("icon-button");
    pVersionBtn->signal_clicked().connect([this] { pAbout_->Show(); });
    pEntryRow_->append(*pVersionBtn);
    pEntryRow_->append(*insEntry_);

    // Construct Settings before FileLoader so the ABI lambda can read a live panel.
    // Append order: open-file left, settings right of it.
    pSettings_= std::make_unique<SettingsPanel>(false, false, false, uiContainer_->has_css_class("compact"));
    pSettings_->signalAbiChanged_.connect([this](bool) { refreshAssemblyDisplay(); });
    pSettings_->signalHexImmChanged_.connect([this](bool on) {
        if(!inst_ || !pFormatUi_) return;
        pFormatUi_->SetHexImm(on);
        pFormatUi_->UpdateDisplay(*inst_);
    });
    pSettings_->signalSaveHistChanged_.connect([this](bool on) {
        // Enabling loads the previous session's history; disabling only stops
        // future writes (the in-memory history stays browsable).
        if(!on) return;
        if(auto loaded= fileMgr_.LoadHistory(history_); !loaded) {
            showError(std::format("history load failed: {}", loaded.error().Msg()));
        }
    });
    pSettings_->signalCompactChanged_.connect([this](bool on) {
        if(on) {
            uiContainer_->remove_css_class("standard");
            uiContainer_->add_css_class("compact");
        } else {
            uiContainer_->remove_css_class("compact");
            uiContainer_->add_css_class("standard");
        }
        // Compact -> allow shrink; Standard -> grow only.
        fitWindowToContent(on);
    });

    // File open — batch decode owned by FileLoader; window takes Instruction ownership.
    pFileLoader_= std::make_unique<FileLoader>(*this, fileMgr_, [this] { return pSettings_->GetAbiOn(); });
    pFileLoader_->signalInput_.connect([this](std::string_view text) { pushInputHistory(text); });
    pFileLoader_->SetOnInstruction([this](std::unique_ptr<Instruction> inst) {
        inst_= std::move(inst);
        showInsResult(*inst_);
    });
    pFileLoader_->signalError_.connect([this](const std::string &msg) { showError(msg); });
    pEntryRow_->append(*pFileLoader_->GetButton());

    // Instruction list (between open-file and settings): non-modal child window.
    pInstList_= std::make_unique<InstListPanel>(*this);
    pInstList_->signalInstruction_.connect([this](std::string_view name, uint32_t word) {
        const auto HEX= std::format("0x{:08X}", word);
        insEntry_->set_text(gui::ToGlibUstring(HEX));
        insEntry_->set_tooltip_text(gui::ToGlibUstring(name));

        inst_= std::make_unique<Instruction>(word, pSettings_->GetAbiOn());
        if(const auto DECODED= inst_->Decode(); !DECODED) {
            showError(std::format("decode failed: {}", DECODED.error().Msg()));
            return;
        }
        pushInputHistory(HEX);
        showInsResult(*inst_);
        // Do not grab_focus here: keep keyboard in the list for multi-pick.
    });
    pEntryRow_->append(*pInstList_->GetButton());
    pEntryRow_->append(*pSettings_->GetMenuButton());

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
    if(pSettings_ && pSettings_->GetSaveHistOn()) {
        if(auto saved= fileMgr_.SaveHistory(history_); !saved) {
            std::println("history save failed: {}", saved.error().Msg());
        }
    }
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

    const bool ABI= pSettings_->GetAbiOn();
    if(PARSED->kind_ == util::InputKind::ASSEMBLY) {
        inst_= std::make_unique<Instruction>(inputStr, ABI);
    } else {
        inst_= std::make_unique<Instruction>(PARSED->word_, ABI);
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
    pFormatUi_->SetHexImm(pSettings_->GetHexImmOn());
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
    inst_         = std::make_unique<Instruction>(VAL, pSettings_->GetAbiOn());
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
    insEntry_->set_text(gui::ToGlibUstring(history_.At(history_.CursorIndex())));
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

    insEntry_->set_text(gui::ToGlibUstring(*STEP));
    insEntry_->set_position(-1);
}

void RISCVInstWindow::loadCSSFromFile()
{
    auto cssProvider= Gtk::CssProvider::create();
    cssProvider->load_from_file(Gio::File::create_for_path(CSS_FILE_PATH));
    if(auto display= Gdk::Display::get_default()) {
        Gtk::StyleContext::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}
