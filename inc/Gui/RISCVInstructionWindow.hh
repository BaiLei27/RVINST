#pragma once

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
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
    Gtk::Popover *pHistoryPopover_ {};
    Gtk::ScrolledWindow *pHistoryScroll_ {};
    Gtk::ListBox *pHistoryList_ {};
    Gtk::Stack *pFormatStack_ {};
    Gtk::Switch *pAbiSwitch_ {};
    Gtk::MenuButton *pIsaMenuBtn_ {};

    std::unique_ptr<Instruction> inst_;
    std::array<InstFormatUI *, 6> formatUi_ {};
    std::deque<std::string> inputHistory_;
    std::string historyDraft_;
    int historyBrowseIndex_ { -1 };

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
    void setupInputHistory();
    void popupInputHistory();
    void pushInputHistory(std::string_view text);
    void rebuildHistoryList();
    void applyHistoryAt(int index);
    void browseHistory(int delta);
    void refreshAssemblyForAbiChange();
    [[nodiscard]] InstFormatUI *uiFor(InstFormat fmt) const noexcept;
    static void loadCSSFromFile();
};

// Date:26/10/07/21:59
