#include <charconv>
#include <cstdint>
#include <format>
#include <print>

#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"
#include "Gui/InstFormatUI.hh"

namespace {

bool isImmAsmToken(std::string_view token) noexcept
{
    // Covers "imm", "imm7" (shift), "shamt", and CSR address "csr".
    return token == "shamt" || token == "csr" || token.starts_with("imm");
}

/// Core always emits decimal immediates; Gui optionally shows them as hex.
std::string formatAsmImm(std::string_view text, bool hex)
{
    if(!hex || text.empty()) return std::string(text);

    std::int64_t value {};
    const auto *const BEGIN= text.data();
    const auto *const END  = BEGIN + text.size();
    auto [ptr, ec]         = std::from_chars(BEGIN, END, value);
    if(ec != std::errc() || ptr != END) return std::string(text);

    if(value < 0) return std::format("-0x{:x}", -value);
    return std::format("0x{:x}", value);
}

} // namespace

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
      pView_(&util::GetInstFormatView(fmt)),
      fmt_(fmt)
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
    pAsmValues_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    pAsmValues_->set_halign(Gtk::Align::START);
    pAsmValues_->set_valign(Gtk::Align::CENTER);
    pAsmValues_->add_css_class("assembly-values");
    rebuildAssemblyDisplay(*pView_);
    appendResultRow("Assembly", *pAsmValues_, &InstFormatUI::GetAssemblyContent);
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
    rebuildBitDiagram(*pView_);
    pFormatCard_->append(*pBitsRow_);
}

void InstFormatUI::clearHoverLinks() noexcept
{
    for(auto &pField: binaryOwned_) {
        if(pField) {
            pField->relatedAsm_.clear();
            pField->relatedBinary_.clear();
        }
    }
    for(auto &pAsm: asmOwned_) {
        if(pAsm) pAsm->relatedBinary_.clear();
    }
}

void InstFormatUI::rebuildBitDiagram(const util::InstFormatView &view)
{
    if(!pBitsRow_) return;

    clearHoverLinks();

    while(Gtk::Widget *pChild= pBitsRow_->get_first_child()) {
        pBitsRow_->remove(*pChild);
    }
    binaryFieldWidgets_.clear();
    binaryOwned_.clear();

    binaryOwned_.reserve(view.fields_.size());
    for(const auto &field: view.fields_) {
        const auto TONE= util::BINARY_FIELD_TONE(field.name_, view.keyFields_, view.fields_);
        auto pBits     = std::make_unique<BinaryFieldWidget>(field, TONE);
        if(auto *pRoot= pBits->GetRoot()) {
            pRoot->set_hexpand(true);
            pRoot->set_halign(Gtk::Align::FILL);
        }
        binaryFieldWidgets_.emplace(std::string(field.name_), pBits.get());
        pBitsRow_->append(*pBits->GetRoot());
        binaryOwned_.emplace_back(std::move(pBits));
    }
}

void InstFormatUI::rebuildAssemblyDisplay(const util::InstFormatView &view)
{
    if(!pAsmValues_) return;

    clearHoverLinks();

    while(Gtk::Widget *pChild= pAsmValues_->get_first_child()) {
        pAsmValues_->remove(*pChild);
    }
    asmFieldWidgets_.clear();
    asmOwned_.clear();

    for(const auto &token: view.asmTokens_) {
        auto pAsm= std::make_unique<AsmMnemonicWidget>(token, pAsmValues_);
        if(token != "," && token != "(" && token != ")") {
            asmFieldWidgets_.emplace(std::string(token), pAsm.get());
        }
        asmOwned_.emplace_back(std::move(pAsm));
    }
}

void InstFormatUI::applyView(const util::InstFormatView &view)
{
    pView_= &view;
    rebuildBitDiagram(view);
    rebuildAssemblyDisplay(view);
    setupHover();
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

void InstFormatUI::SetHexImm(bool on) noexcept { hasHexImm_= on; }

void InstFormatUI::UpdateDisplay(const Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType) return;

    const auto FMT= pType->GetInstFormat();
    auto iShape   = ItypeImmShape::IMM12;
    auto rShape   = RtypeShape::BASE;
    if(InstFormat::I == FMT) {
        const auto &L= pType->GetInstLayout();
        iShape       = DetectIImmShape(L.I.opc, L.I.fct3);
    } else if(InstFormat::R == FMT) {
        const auto &L= pType->GetInstLayout();
        rShape       = DetectRShape(L.R.opc);
    }

    const bool SHAPE_CHANGED=
        (InstFormat::I == FMT && iShape != iShape_) || (InstFormat::R == FMT && rShape != rShape_);
    if(FMT != fmt_ || SHAPE_CHANGED) {
        fmt_   = FMT;
        iShape_= iShape;
        rShape_= rShape;
        if(InstFormat::I == FMT) applyView(util::GetIInstFormatView(iShape));
        else if(InstFormat::R == FMT) applyView(util::GetRInstFormatView(rShape));
        else applyView(util::GetInstFormatView(FMT));
    }

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

    for(auto &[name, pAsm]: asmFieldWidgets_) {
        if(pAsm && pAsm->GetLabel() && name != "mnemonic") pAsm->GetLabel()->set_text("");
    }

    const auto &parts= pType->GetInstAssembly();
    size_t j         = 0;

    for(const auto &token: pView_->asmTokens_) {
        if(token == "," || token == "(") continue; // Punctuation is owned by the format view, not by operand values.

        while(j < parts.size()
              && (parts[j] == " "
                  || parts[j] == ","
                  || parts[j] == "(")) {
            ++j;
        }

        auto it= asmFieldWidgets_.find(token);
        if(it != asmFieldWidgets_.end() && it->second->GetLabel() && j < parts.size()) { // NOLINT
            const auto &part= parts[j++];
            it->second->GetLabel()->set_text(isImmAsmToken(token) ? formatAsmImm(part, hasHexImm_) : part);
        }
    }

    // C-Type view only exposes a mnemonic chip — fold the full operand list into it.
    if(1 == pView_->asmTokens_.size() && pView_->asmTokens_.front() == "mnemonic") {
        auto it= asmFieldWidgets_.find(std::string_view { "mnemonic" });
        if(it != asmFieldWidgets_.end() && it->second->GetLabel()) {
            std::string full;
            for(const auto &p: parts) full+= p;
            it->second->GetLabel()->set_text(full);
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
        if(token == "(" || token == ")") {
            result+= std::string(token);
            continue;
        }

        auto it= asmFieldWidgets_.find(token);
        if(it == asmFieldWidgets_.end() || !it->second->GetLabel()) continue; // NOLINT

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

bool InstFormatUI::FitToContent()
{
    if(!pBitsRow_) return false;

    // Temporarily unexpand fields to measure intrinsic bit-diagram width.
    for(auto &pField: binaryOwned_) {
        if(pField && pField->GetRoot()) pField->GetRoot()->set_hexpand(false);
    }
    pBitsRow_->set_hexpand(false);
    return true;
}

int InstFormatUI::MeasureWidth() const
{
    if(!pBitsRow_) return 0;

    int minW= 0, natW= 0, minBase= -1, natBase= -1;
    pBitsRow_->measure(Gtk::Orientation::HORIZONTAL, -1, minW, natW, minBase, natBase);
    return natW;
}

void InstFormatUI::RestoreHexPand()
{
    if(!pBitsRow_) return;

    for(auto &pField: binaryOwned_) {
        if(pField && pField->GetRoot()) pField->GetRoot()->set_hexpand(true);
    }
    pBitsRow_->set_hexpand(true);
    pBitsRow_->queue_resize();
}
