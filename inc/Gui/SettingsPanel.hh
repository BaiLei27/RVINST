#pragma once

#include <gtkmm.h>
#include <sigc++/signal.h>

/**
 * @brief Owns the settings popover and its toggles (ABI, hex immediates,
 *        save-history, compact font).
 *
 * Pure widget layer: holds the toggle state and emits a change signal per
 * toggle. RISCVInstWindow subscribes and applies the side effects, so this
 * class never depends on the window or the parse backend.
 */
class SettingsPanel {
private:
    Gtk::MenuButton *PMenuBtn_ {};
    bool AbiOn_ {};
    bool HexImmOn_ {};
    bool SaveHistOn_ {};
    bool CompactOn_ {};

public:
    sigc::signal<void(bool)> signalAbiChanged_;
    sigc::signal<void(bool)> signalHexImmChanged_;
    sigc::signal<void(bool)> signalSaveHistChanged_;
    sigc::signal<void(bool)> signalCompactChanged_;

public:
    SettingsPanel(bool abiOn, bool hexImmOn, bool saveHistOn, bool compactOn);

public:
    [[nodiscard]] Gtk::MenuButton *MenuButton() const noexcept;

    [[nodiscard]] bool AbiOn() const noexcept;

    [[nodiscard]] bool HexImmOn() const noexcept;

    [[nodiscard]] bool SaveHistOn() const noexcept;

    [[nodiscard]] bool CompactOn() const noexcept;

private:
    static void addToggle(Gtk::Box &box,
                          const char *pLabel,
                          const char *pCssClass,
                          bool &state,
                          sigc::signal<void(bool)> &signal);
};

// Date:26/10/19
