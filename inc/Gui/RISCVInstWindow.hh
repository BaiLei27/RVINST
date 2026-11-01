#pragma once

#include "Core/Instruction.hh"
#include "Gui/AboutWindow.hh"
#include "Gui/FileLoader.hh"
#include "Gui/InstFormatUI.hh"
#include "Gui/InstListPanel.hh"
#include "Gui/SettingsPanel.hh"
#include "Util/FileIo.hh"
#include "Util/InputHistoryMgr.hh"

class RISCVInstWindow: public Gtk::Window {
public:
    Gtk::Box *uiContainer_ {};
    Gtk::Box *pEntryRow_ {};
    Gtk::Entry *insEntry_ {};
    Gtk::Button *insButtonParse_ {};
    Gtk::Label *pErrorLabel_ {};
    Gtk::Box *pErrorCard_ {};
    Gtk::Stack *pFormatStack_ {};
    Gtk::Popover *pHistoryPopover_ {};
    Gtk::ScrolledWindow *pHistoryScroll_ {};
    Gtk::ListBox *pHistoryList_ {};
    InstFormatUI *pFormatUi_ {};

    // Component collaborators (buttons/switches abstracted out)
    util::InputHistoryMgr history_;
    util::FileIo fileMgr_;
    std::unique_ptr<SettingsPanel> pSettings_;
    std::unique_ptr<FileLoader> pFileLoader_;
    std::unique_ptr<InstListPanel> pInstList_;
    std::unique_ptr<AboutWindow> pAbout_;
    std::unique_ptr<Instruction> inst_;

public:
    RISCVInstWindow();
    ~RISCVInstWindow() override;

private:
    // Parsing & result display
    void onInsButtonParseClicked();
    void showInsResult(Instruction &inst);
    void showError(const std::string &message);
    void refreshAssemblyDisplay();
    void fitWindowToContent(bool allowShrink);

    // Format UI panel
    void initInstFormatUI();
    void hideFormatUI();

    // Input history (model logic delegated to InputHistoryMgr)
    void setupInputHistory();
    void popupInputHistory();
    void pushInputHistory(std::string_view text);
    void rebuildHistoryList();
    void applyHistoryAt(size_t index);
    void browseHistory(int delta);

    // Misc
    static void loadCSSFromFile();

    // Widgets owned by the window (component pointers owned elsewhere)
};

// Date:26/10/07/21:59
