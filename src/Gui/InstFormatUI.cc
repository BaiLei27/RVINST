#include "Gui/AsmMnemonicWidget.hh"
#include "Gui/BinaryFieldWidget.hh"
#include "Gui/InstFormatUI.hh"

namespace {

Gtk::Box *makeLabeledRow(const char *pTitle)
{
    auto *pRow     = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    auto *pTitleLbl= Gtk::make_managed<Gtk::Label>(pTitle);
    pTitleLbl->set_margin_end(8);
    pRow->set_hexpand(true);
    pRow->append(*pTitleLbl);
    return pRow;
}

void appendCopyAndToViewButtons(Gtk::Box &row, InstFormatUI *pUI, std::string (InstFormatUI::*getter)() const)
{
    auto *pSpacer= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    pSpacer->set_hexpand(true);

    auto *pCopyBtn  = Gtk::make_managed<Gtk::Button>("Copy");
    auto *pToViewBtn= Gtk::make_managed<Gtk::Button>("To View");
    pCopyBtn->set_margin_start(8);
    pToViewBtn->set_margin_start(4);

    pCopyBtn->signal_clicked().connect([pUI, getter] {
        auto content= (pUI->*getter)();
        if(content.empty()) return;

        if(auto display= Gdk::Display::get_default()) {
            if(auto clipboard= display->get_clipboard()) clipboard->set_text(Glib::ustring(content));
        }
    });
    pToViewBtn->signal_clicked().connect([pUI, getter] {
        auto content= (pUI->*getter)();
        if(!content.empty()) pUI->signalOutput_.emit(content);
    });

    row.append(*pSpacer);
    row.append(*pCopyBtn);
    row.append(*pToViewBtn);
}

} // namespace

InstFormatUI::InstFormatUI(InstFormat fmt)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 10),
      pView_(&util::GetInstFormatView(fmt))
{
    setupAssemblyDisplay();
    setupBinaryDisplay();
    setupHexDisplay();
    setupHover();
}

void InstFormatUI::setupAssemblyDisplay()
{
    auto *pAsmArea= makeLabeledRow("Assembly =");

    for(const auto &token: pView_->asmTokens_) {
        auto pAsm= std::make_unique<AsmMnemonicWidget>(token, pAsmArea);
        if(token != ",") asmFieldWidgets_.emplace(std::string(token), pAsm.get());
        asmOwned_.emplace_back(std::move(pAsm));
    }

    appendCopyAndToViewButtons(*pAsmArea, this, &InstFormatUI::GetAssemblyContent);
    append(*pAsmArea);
}

void InstFormatUI::setupBinaryDisplay()
{
    auto *pBinaryArea= makeLabeledRow("Binary =");
    pBinaryArea->set_halign(Gtk::Align::FILL);
    pBinaryArea->set_valign(Gtk::Align::CENTER);
    pBinaryArea->set_margin_bottom(22);

    int nibbleIndex= 0;
    binaryOwned_.reserve(pView_->fields_.size());
    for(const auto &field: pView_->fields_) {
        auto pBits= std::make_unique<BinaryFieldWidget>(field, nibbleIndex);
        binaryFieldWidgets_.emplace(std::string(field.name_), pBits.get());
        pBinaryArea->append(*pBits->GetRoot());
        binaryOwned_.emplace_back(std::move(pBits));
    }

    appendCopyAndToViewButtons(*pBinaryArea, this, &InstFormatUI::GetBinaryContent);
    append(*pBinaryArea);
}

void InstFormatUI::setupHexDisplay()
{
    auto *pHexArea= makeLabeledRow("Hexadecimal =");

    pHexLabel_= Gtk::make_managed<Gtk::Label>("0x00000000");
    pHexLabel_->set_margin_end(8);
    pHexArea->append(*pHexLabel_);

    appendCopyAndToViewButtons(*pHexArea, this, &InstFormatUI::GetHexContent);
    append(*pHexArea);
}

void InstFormatUI::setupHover()
{
    for(auto &[name, pAsm]: asmFieldWidgets_)
        pAsm->SetupHover(name, *pView_, binaryFieldWidgets_);
    for(auto &[name, pBin]: binaryFieldWidgets_)
        pBin->SetupHover(name, *pView_, binaryFieldWidgets_, asmFieldWidgets_);
}

void InstFormatUI::UpdateDisplay(const Instruction &inst)
{
    updateAssemblyDisplay(inst);
    updateBinaryDisplay(inst);
    updateHexDisplay(inst);
}

void InstFormatUI::updateAssemblyDisplay(const Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType)
        return;

    const auto &parts= pType->GetInstAssembly();
    size_t j         = 0;
    for(const auto &token: pView_->asmTokens_) {
        if(token == ",") continue;

        while(j < parts.size() && (parts[j] == " " || parts[j] == ","))
            ++j;

        auto it= asmFieldWidgets_.find(token);
        if(it != asmFieldWidgets_.end()
           && it->second->GetLabel()
           && j < parts.size()) it->second->GetLabel()->set_text(parts[j++]);
    }
}

void InstFormatUI::updateBinaryDisplay(const Instruction &inst)
{
    const auto *pType= inst.GetTypePtr();
    if(!pType) return;

    const auto &bits= pType->GetInstBitsField();
    const auto N    = binaryOwned_.size();
    const auto M    = bits.size();
    for(size_t i= 0; i < N && i < M; ++i)
        binaryOwned_[N - 1 - i]->UpdateBits(bits[i]);
}

void InstFormatUI::updateHexDisplay(const Instruction &inst)
{
    if(pHexLabel_)
        pHexLabel_->set_text("0x" + inst.GetHexStr());
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
        if(token == "mnemonic")
            result+= " ";
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
