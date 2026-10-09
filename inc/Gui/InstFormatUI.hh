#pragma once

#include <memory>
#include <string>
#include <vector>
#include <gtkmm.h>

#include "Core/Instruction.hh"
#include "Gui/InstCommST.hh"
#include "ISA/InstFormat.hh"
#include "Util/InstFormatView.hpp"

class InstFormatUI: public Gtk::Box {
public:
    const util::InstFormatView *pView_ {};
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
    int resultRowIndex_ {};
    InstCommST::BinaryFieldWidgetMap_u binaryFieldWidgets_;
    InstCommST::AsmMnemonicWidgetMap_u asmFieldWidgets_;
    std::vector<std::unique_ptr<class BinaryFieldWidget>> binaryOwned_;
    std::vector<std::unique_ptr<class AsmMnemonicWidget>> asmOwned_;

public:
    sigc::signal<void(const std::string &)> signalOutput_;

    explicit InstFormatUI(InstFormat fmt);
    void UpdateDisplay(const Instruction &inst);

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
