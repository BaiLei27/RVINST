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
    Gtk::Label *pHexLabel_ {};
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
    void setupAssemblyDisplay();
    void setupBinaryDisplay();
    void setupHexDisplay();
    void setupHover();
    void updateAssemblyDisplay(const Instruction &inst);
    void updateBinaryDisplay(const Instruction &inst);
    void updateHexDisplay(const Instruction &inst);
};

// Date:26/10/07/21:59
