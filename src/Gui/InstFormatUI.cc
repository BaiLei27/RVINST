#include <format>
#include <print>

#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"
#include "Gui/InstFormatUI.hh"

void InstFormatUI::appendResultRow(const char *pTitle,
                                   Gtk::Widget &value,
                                   std::string (InstFormatUI::*getter)() const,
                                   bool last)
{
    const int ROW= resultRowIndex_++;

    auto *pTitleLbl= Gtk::make_managed<Gtk::Label>(pTitle);
    pTitleLbl->set_halign(Gtk::Align::START);
    pTitleLbl->set_valign(Gtk::Align::CENTER);
    pTitleLbl->set_xalign(0.0F);
    pTitleLbl->set_width_chars(12);
    pTitleLbl->add_css_class("result-row-title");

    value.set_halign(Gtk::Align::START);
    value.set_valign(Gtk::Align::CENTER);
    value.set_hexpand(true);

    auto *pActions= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
    pActions->set_halign(Gtk::Align::END);
    pActions->set_valign(Gtk::Align::CENTER);

    auto *pUseBtn= Gtk::make_managed<Gtk::Button>();
    pUseBtn->set_icon_name("document-edit-symbolic");
    pUseBtn->set_tooltip_text("Use as input");
    pUseBtn->add_css_class("use-input-button");
    pUseBtn->signal_clicked().connect([this, getter] {
        const auto CONTENT= (this->*getter)();
        if(!CONTENT.empty()) signalOutput_.emit(CONTENT);
    });

    auto *pCopyBtn= Gtk::make_managed<Gtk::Button>();
    pCopyBtn->set_icon_name("edit-copy-symbolic");
    pCopyBtn->set_tooltip_text("Copy");
    pCopyBtn->add_css_class("copy-icon-button");
    pCopyBtn->signal_clicked().connect([this, getter] {
        const auto CONTENT= (this->*getter)();
        if(CONTENT.empty()) return;

        if(auto display= Gdk::Display::get_default()) {
            if(auto clipboard= display->get_clipboard()) {
                clipboard->set_text(Glib::ustring(CONTENT));
            }
        }
    });

    pActions->append(*pUseBtn);
    pActions->append(*pCopyBtn);

    auto *pRowBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    pRowBox->add_css_class("result-row");
    if(last) pRowBox->add_css_class("result-row-last");
    pRowBox->set_hexpand(true);
    pRowBox->append(*pTitleLbl);
    pRowBox->append(value);
    pRowBox->append(*pActions);

    pResultGrid_->attach(*pRowBox, 0, ROW);
}

InstFormatUI::InstFormatUI(InstFormat fmt)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 14),
      pView_(&util::GetInstFormatView(fmt))
{
    add_css_class("format-panel");
    setupResultCard();
    setupFormatCard();
    setupHover();
}

void InstFormatUI::setupResultCard()
{
    pResultCard_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
    pResultCard_->add_css_class("result-card");

    pResultGrid_= Gtk::make_managed<Gtk::Grid>();
    pResultGrid_->set_hexpand(true);
    pResultGrid_->add_css_class("result-grid");
    resultRowIndex_= 0;
    pResultCard_->append(*pResultGrid_);

    setupAssemblyDisplay();
    setupBinaryTextDisplay();
    setupHexDisplay();

    append(*pResultCard_);
}

void InstFormatUI::setupFormatCard()
{
    pFormatCard_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    pFormatCard_->set_hexpand(true);
    pFormatCard_->add_css_class("format-card");

    auto *pHeader= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    pHeader->add_css_class("format-card-header");

    auto *pMetaCol= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    pMetaCol->set_halign(Gtk::Align::START);

    auto makeKeyValueRow= [](const char *pKey, Gtk::Label *&pValueOut) {
        auto *pRow   = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
        auto *pKeyLbl= Gtk::make_managed<Gtk::Label>(pKey);
        pKeyLbl->set_halign(Gtk::Align::START);
        pKeyLbl->set_valign(Gtk::Align::CENTER);
        pKeyLbl->set_width_chars(7);
        pKeyLbl->set_xalign(0.0F);
        pKeyLbl->add_css_class("meta-key-label");

        pValueOut= Gtk::make_managed<Gtk::Label>("—");
        pValueOut->set_halign(Gtk::Align::START);
        pValueOut->set_valign(Gtk::Align::CENTER);
        pValueOut->add_css_class("info-badge");

        pRow->append(*pKeyLbl);
        pRow->append(*pValueOut);
        return pRow;
    };

    pMetaCol->append(*makeKeyValueRow("Format", pFormatBadge_));
    pMetaCol->append(*makeKeyValueRow("ISA", pIsaBadge_));
    pHeader->append(*pMetaCol);

    auto *pSpacer= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    pSpacer->set_hexpand(true);
    pHeader->append(*pSpacer);

    pOpcodeBadge_= Gtk::make_managed<Gtk::Label>("opcode: —");
    pOpcodeBadge_->add_css_class("opcode-badge");
    pOpcodeBadge_->set_halign(Gtk::Align::END);
    pOpcodeBadge_->set_valign(Gtk::Align::START);
    pHeader->append(*pOpcodeBadge_);

    pFormatCard_->append(*pHeader);

    auto *pMetaRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    pMetaRow->add_css_class("format-meta-row");
    auto *pManualTitle= Gtk::make_managed<Gtk::Label>("Manual");
    pManualTitle->add_css_class("format-meta-label");
    pManualTitle->set_halign(Gtk::Align::START);
    pManualTitle->set_valign(Gtk::Align::CENTER);
    pMetaRow->append(*pManualTitle);

    pManualLink_= Gtk::make_managed<Gtk::Label>("—");
    pManualLink_->set_halign(Gtk::Align::START);
    pManualLink_->set_valign(Gtk::Align::CENTER);
    pManualLink_->set_hexpand(true);
    pManualLink_->set_xalign(0.0F);
    pManualLink_->set_wrap(false);
    pManualLink_->set_ellipsize(Pango::EllipsizeMode::END);
    pManualLink_->set_selectable(true);
    pManualLink_->add_css_class("manual-link");
    pManualLink_->signal_activate_link().connect(
        [](const Glib::ustring &uri) -> bool {
            try {
                Gio::AppInfo::launch_default_for_uri(uri);
            } catch(const Glib::Error &e) {
                std::println(stderr, "Open manual failed: {}", e.what());
            }
            return true;
        },
        false);
    pMetaRow->append(*pManualLink_);

    pFormatCard_->append(*pMetaRow);
    setupBitDiagram();
    append(*pFormatCard_);
}

void InstFormatUI::setupAssemblyDisplay()
{
    auto *pAsmValues= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    pAsmValues->set_halign(Gtk::Align::START);
    pAsmValues->set_valign(Gtk::Align::CENTER);
    pAsmValues->add_css_class("assembly-values");

    for(const auto &token: pView_->asmTokens_) {
        auto pAsm= std::make_unique<AsmMnemonicWidget>(token, pAsmValues);
        if(token != ",") {
            asmFieldWidgets_.emplace(std::string(token), pAsm.get());
        }
        asmOwned_.emplace_back(std::move(pAsm));
    }

    appendResultRow("Assembly", *pAsmValues, &InstFormatUI::GetAssemblyContent);
}

void InstFormatUI::setupBinaryTextDisplay()
{
    pBinaryLabel_= Gtk::make_managed<Gtk::Label>("—");
    pBinaryLabel_->set_ellipsize(Pango::EllipsizeMode::END);
    pBinaryLabel_->set_xalign(0.0F);
    pBinaryLabel_->add_css_class("binary-value");
    pBinaryLabel_->add_css_class("value-chip");
    appendResultRow("Binary", *pBinaryLabel_, &InstFormatUI::GetBinaryContent);
}

void InstFormatUI::setupHexDisplay()
{
    pHexLabel_= Gtk::make_managed<Gtk::Label>("0x00000000");
    pHexLabel_->set_xalign(0.0F);
    pHexLabel_->add_css_class("hex-value");
    pHexLabel_->add_css_class("value-chip");
    appendResultRow("Hexadecimal", *pHexLabel_, &InstFormatUI::GetHexContent, true);
}

void InstFormatUI::setupBitDiagram()
{
    pBitsRow_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    pBitsRow_->set_halign(Gtk::Align::FILL);
    pBitsRow_->set_hexpand(true);
    pBitsRow_->add_css_class("bit-diagram-row");

    binaryOwned_.reserve(pView_->fields_.size());
    int tone= 0;
    for(const auto &field: pView_->fields_) {
        auto pBits= std::make_unique<BinaryFieldWidget>(field, tone++);
        if(auto *pRoot= pBits->GetRoot()) {
            pRoot->set_hexpand(true);
            pRoot->set_halign(Gtk::Align::FILL);
        }
        binaryFieldWidgets_.emplace(std::string(field.name_), pBits.get());
        pBitsRow_->append(*pBits->GetRoot());
        binaryOwned_.emplace_back(std::move(pBits));
    }

    pFormatCard_->append(*pBitsRow_);
}

void InstFormatUI::setupHover()
{
    for(auto &[name, pAsm]: asmFieldWidgets_) {
        pAsm->SetupHover(name, *pView_, binaryFieldWidgets_);
    }
    for(auto &[name, pBin]: binaryFieldWidgets_) {
        pBin->SetupHover(name, *pView_, binaryFieldWidgets_, asmFieldWidgets_);
    }
}

void InstFormatUI::UpdateDisplay(const Instruction &inst)
{
    updateHeader(inst);
    updateAssemblyDisplay(inst);
    updateBinaryDisplay(inst);
    updateHexDisplay(inst);
}

void InstFormatUI::updateHeader(const Instruction &inst)
{
    if(pFormatBadge_) {
        pFormatBadge_->set_text(Glib::ustring(inst.GetFormat().data(), inst.GetFormat().size()));
    }
    if(pIsaBadge_) {
        pIsaBadge_->set_text(Glib::ustring(inst.GetXLEN().data(), inst.GetXLEN().size()));
    }
    if(pOpcodeBadge_) {
        const auto *pType= inst.GetTypePtr();
        if(pType) {
            pOpcodeBadge_->set_text(std::format("opcode: 0x{:02x}", pType->GetInstOpcode()));
        }
    }
    if(pManualLink_) {
        const auto MANUAL= inst.GetManual();
        const auto NAME  = inst.GetName();
        const Glib::ustring HREF(MANUAL.data(), MANUAL.size());
        const Glib::ustring DISP=
            NAME.empty() ? Glib::ustring("—") : Glib::ustring(NAME.data(), NAME.size());
        const bool IS_URL=
            MANUAL.starts_with("http://") || MANUAL.starts_with("https://");
        if(IS_URL) {
            const auto MARKUP=
                std::format("<a href=\"{}\">{}</a>",
                            Glib::Markup::escape_text(HREF).raw(),
                            Glib::Markup::escape_text(DISP).raw());
            pManualLink_->set_markup(MARKUP);
            pManualLink_->set_tooltip_text(HREF);
        } else {
            pManualLink_->set_text(DISP);
            pManualLink_->set_tooltip_text(HREF.empty() ? "" : HREF);
        }
    }
}

void InstFormatUI::updateAssemblyDisplay(const Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType) return;

    const auto &parts= pType->GetInstAssembly();
    size_t j         = 0;

    for(const auto &token: pView_->asmTokens_) {
        if(token == ",") continue;

        while(j < parts.size() && (parts[j] == " " || parts[j] == ",")) {
            ++j;
        }

        auto it= asmFieldWidgets_.find(token);
        if(it != asmFieldWidgets_.end() && it->second->GetLabel() && j < parts.size()) {
            it->second->GetLabel()->set_text(parts[j++]);
        }
    }
}

void InstFormatUI::updateBinaryDisplay(const Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType) return;

    const auto &bits= pType->GetInstBitsField();
    const auto N    = binaryOwned_.size();
    const auto M    = bits.size();

    for(size_t i= 0; i < N && i < M; ++i) {
        binaryOwned_[N - 1 - i]->UpdateBits(bits[i]);
    }

    if(pBinaryLabel_) {
        pBinaryLabel_->set_text(GetBinaryContent());
    }
}

void InstFormatUI::updateHexDisplay(const Instruction &inst)
{
    if(pHexLabel_) {
        pHexLabel_->set_text("0x" + inst.GetHexStr());
    }
}

std::string InstFormatUI::GetAssemblyContent() const
{
    std::string result;
    for(const auto &token: pView_->asmTokens_) {
        if(token == ",") {
            result+= ", ";
            continue;
        }

        auto it= asmFieldWidgets_.find(token);
        if(it == asmFieldWidgets_.end() || !it->second->GetLabel()) continue;

        result+= it->second->GetLabel()->get_text().raw();
        if(token == "mnemonic") result+= " ";
    }
    return result;
}

std::string InstFormatUI::GetBinaryContent() const
{
    std::string result;
    result.reserve(32);

    for(const auto &pField: binaryOwned_) {
        for(auto *pLabel: pField->GetLabels()) {
            if(pLabel) result+= pLabel->get_text().raw();
        }
    }
    return result;
}

std::string InstFormatUI::GetHexContent() const
{
    return pHexLabel_ ? std::string(pHexLabel_->get_text().raw()) : std::string {};
}
