#pragma once

#include "Core/Instruction.hh"
#include "Gui/InstFormatUI.hh"
#include "Gui/InputHistoryController.hh"

class RISCVInstructionWindow: public Gtk::Window {
public:
    std::unique_ptr<Gtk::Window> pVersionWindow_;
    std::unique_ptr<Instruction> inst_;
    InputHistoryController history_;
    Gtk::Box *uiContainer_ {};
    Gtk::Box *pEntryRow_ {};
    Gtk::Entry *insEntry_ {};
    Gtk::Button *insButtonParse_ {};
    Gtk::Label *pErrorLabel_ {};
    Gtk::Box *pErrorCard_ {};
    Gtk::MenuButton *pSettingsBtn_ {};
    Gtk::Button *pVersionBtn_ {};
    Gtk::Popover *pHistoryPopover_ {};
    Gtk::ScrolledWindow *pHistoryScroll_ {};
    Gtk::ListBox *pHistoryList_ {};
    Gtk::Stack *pFormatStack_ {};
    Gtk::Switch *pAbiSwitch_ {};
    Gtk::Switch *pFontSizeSwitch_ {};
    Gtk::Switch *pHexImmSwitch_ {};
    Gtk::DropDown *pIsaDrop_ {};
    InstFormatUI *pFormatUi_ {};
    bool hasSetABI_ {};
    bool hasHexImm_ {};

public:
    RISCVInstructionWindow();
    ~RISCVInstructionWindow() override;

private:
    // Instruction parsing & result display
    void onInsButtonParseClicked();
    void showInsResult(Instruction &inst);
    void showError(const std::string &message);
    void refreshAssemblyDisplay();
    void fitWindowToContent(bool allowShrink);

    // Format UI panel
    void initInstFormatUI();
    void hideFormatUI();

    // Settings & version info
    void setupSettingsPopover();
    void showVersionWindow();

    // Input history (model logic delegated to InputHistoryController)
    void setupInputHistory();
    void popupInputHistory();
    void pushInputHistory(std::string_view text);
    void rebuildHistoryList();
    void applyHistoryAt(size_t index);
    void browseHistory(int delta);

    // Misc
    static void loadCSSFromFile();
};

// Date:26/10/07/21:59
