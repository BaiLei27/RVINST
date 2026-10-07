#pragma once

#include <array>
#include <memory>
#include <string>
#include <gtkmm.h>

#include "Core/Instruction.hh"
#include "Gui/InstFormatUI.hh"
#include "ISA/InstFormat.hh"

class RISCVInstructionWindow: public Gtk::Window {
public:
    Gtk::Box *uiContainer_ {};
    Gtk::Box *pEntryRow_ {};
    Gtk::Entry *insEntry_ {};
    Gtk::Button *insButtonParse_ {};
    Gtk::TextView *insTextView_ {};
    Gtk::Button *pSettingsBtn_ {};
    Gtk::Popover *pSettingsPopover_ {};
    Gtk::Switch *pAbiSwitch_ {};
    Gtk::MenuButton *pIsaMenuBtn_ {};
    std::unique_ptr<Instruction> inst_;
    std::array<InstFormatUI *, 6> formatUi_ {};

    bool hasSetABI_ {};
    int selectedIsaIndex_ {};

public:
    RISCVInstructionWindow();

private:
    void onInsButtonParseClicked();
    void showInsResult(Instruction &inst);
    void showError(const std::string &message);

    void initInstFormatUI();
    void hideAllTypeUI();
    void setupSettingsPopover();
    void refreshAssemblyForAbiChange();
    [[nodiscard]] InstFormatUI *uiFor(InstFormat fmt) const noexcept;
    static void loadCSSFromFile();
};

// Date:26/10/07/21:59
