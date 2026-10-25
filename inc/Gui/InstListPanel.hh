#pragma once

#include <gtkmm.h>
#include <sigc++/signal.h>

#include <cstdint>
#include <memory>
#include <string_view>

/**
 * @brief Toggle button + non-modal child window of supported instructions.
 *
 * Pattern matches AboutWindow: a separate Gtk::Window owned here, transient to
 * the main window, draggable. modal=false so the main encoder stays usable while
 * the list is open. Groups by ISA module; within a module, order matches Core
 * G_INST_TABLE / GetAllInstructions.
 */
class InstListPanel {

private:
    Gtk::Window *PParent_ {}; // non-owning
    Gtk::ToggleButton *PBtn_ {};
    std::unique_ptr<Gtk::Window> PWindow_;
    Gtk::Box *PPanel_ {};
    Gtk::ScrolledWindow *PScroll_ {};
    bool SyncingBtn_ {};

public:
    explicit InstListPanel(Gtk::Window &parent);

public:
    [[nodiscard]] Gtk::ToggleButton *GetButton() const noexcept;

    /// label: mnemonic for the entry; word: default encoding to decode
    sigc::signal<void(std::string_view label, uint32_t word)> signalInstruction_;

private:
    void build();
    void showWindow();
    void hideWindow();
    void setButtonActive(bool active);
    void onToggled();
    void onInstructionActivated(std::string_view name, uint32_t word);
};

// Date:26-10-25-22:31
