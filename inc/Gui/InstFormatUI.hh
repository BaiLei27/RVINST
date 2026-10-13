#pragma once

#include <string>
#include <vector>
#include <gtkmm.h>

#include "Core/Instruction.hh"
#include "Gui/InstCommST.hh"
#include "ISA/InstFormat.hh"
#include "Util/InstFormatView.hh"

class InstFormatUI: public Gtk::Box {
public:
    std::vector<std::unique_ptr<class BinaryFieldWidget>> binaryOwned_;
    std::vector<std::unique_ptr<class AsmMnemonicWidget>> asmOwned_;
    sigc::signal<void(const std::string &)> signalOutput_;
    InstCommST::BinaryFieldWidgetMap_u binaryFieldWidgets_;
    InstCommST::AsmMnemonicWidgetMap_u asmFieldWidgets_;

    Gtk::Box *pResultCard_ {};
    Gtk::Grid *pResultGrid_ {};
    Gtk::Box *pFormatCard_ {};
    Gtk::Label *pFormatBadge_ {};
    Gtk::Label *pIsaBadge_ {};
    Gtk::Label *pOpcodeBadge_ {};
    Gtk::Label *pManualLink_ {};
    Gtk::Label *pHexLabel_ {};
    Gtk::Label *pBinaryLabel_ {};
    Gtk::Box *pBitsRow_ {};
    Gtk::Box *pAsmValues_ {};
    const util::InstFormatView *pView_ {};
    int resultRowIndex_ {};
    InstFormat fmt_ { InstFormat::UNKNOWN };
    ItypeImmShape iShape_ { ItypeImmShape::IMM12 };

public:
    explicit InstFormatUI(InstFormat fmt);
    void UpdateDisplay(const Instruction &inst);

public:
    void RestoreHexPand();

    [[nodiscard]] bool FitToContent();
    [[nodiscard]] int MeasureWidth() const;
    [[nodiscard]] std::string GetAssemblyContent() const;
    [[nodiscard]] std::string GetBinaryContent() const;
    [[nodiscard]] std::string GetHexContent() const;

private:
    void setupResultCard();
    void setupFormatCard();
    void setupAssemblyDisplay();
    void setupBinaryTextDisplay();
    void setupHexDisplay();
    void setupBitDiagram();
    void setupHover();
    void rebuildBitDiagram(const util::InstFormatView &view);
    void rebuildAssemblyDisplay(const util::InstFormatView &view);
    void applyView(const util::InstFormatView &view);
    void updateAssemblyDisplay(const Instruction &inst);
    void updateBinaryDisplay(const Instruction &inst);
    void updateHexDisplay(const Instruction &inst);
    void updateHeader(const Instruction &inst);

    void appendResultRow(const char *pTitle,
                         Gtk::Widget &value,
                         std::string (InstFormatUI::*getter)() const,
                         bool last= false);
};

// Date:26/10/07/21:59
