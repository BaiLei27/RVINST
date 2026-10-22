#include "Gui/SettingsPanel.hh"

Gtk::MenuButton *SettingsPanel::GetMenuButton() const noexcept { return PMenuBtn_; }

bool SettingsPanel::GetAbiOn() const noexcept { return AbiOn_; }

bool SettingsPanel::GetHexImmOn() const noexcept { return HexImmOn_; }

bool SettingsPanel::GetSaveHistOn() const noexcept { return SaveHistOn_; }

bool SettingsPanel::GetCompactOn() const noexcept { return CompactOn_; }

SettingsPanel::SettingsPanel(bool abiOn, bool hexImmOn, bool saveHistOn, bool compactOn)
    : PMenuBtn_(Gtk::make_managed<Gtk::MenuButton>()),
      AbiOn_(abiOn),
      HexImmOn_(hexImmOn),
      SaveHistOn_(saveHistOn),
      CompactOn_(compactOn)
{
    PMenuBtn_->set_icon_name("emblem-system-symbolic");
    PMenuBtn_->set_tooltip_text("ABI / display settings");
    PMenuBtn_->set_always_show_arrow(false);
    PMenuBtn_->add_css_class("icon-button");

    // MenuButton owns the popover; avoids Button+set_parent press-state bleed
    // into the settings icon when clicking ABI controls inside the popover.
    auto *pPopover= Gtk::make_managed<Gtk::Popover>();
    pPopover->set_has_arrow(true);
    pPopover->add_css_class("settings-popover");

    auto *pBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    pBox->set_margin(14);
    pBox->set_hexpand(true);
    pBox->add_css_class("settings-content");

    addToggle(*pBox, "ABI", "abi-switch", AbiOn_, signalAbiChanged_);
    addToggle(*pBox, "Hex imm", "hex-imm-switch", HexImmOn_, signalHexImmChanged_);
    addToggle(*pBox, "Save history", "save-hist-switch", SaveHistOn_, signalSaveHistChanged_);
    addToggle(*pBox, "Compact", "font-size-switch", CompactOn_, signalCompactChanged_);

    pPopover->set_child(*pBox);
    PMenuBtn_->set_popover(*pPopover);
}

void SettingsPanel::addToggle(Gtk::Box &box,
                              const char *pLabel,
                              const char *pCssClass,
                              bool &state,
                              sigc::signal<void(bool)> &signal)
{
    auto *pRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    pRow->set_halign(Gtk::Align::FILL);
    pRow->set_hexpand(true);
    pRow->add_css_class("settings-row");

    auto *pName= Gtk::make_managed<Gtk::Label>(pLabel);
    pName->set_halign(Gtk::Align::START);
    pName->set_hexpand(true); // absorb leftover width so the switch stays right-aligned
    pName->add_css_class("settings-label");
    pRow->append(*pName);

    auto *pSwitch= Gtk::make_managed<Gtk::Switch>();
    pSwitch->set_active(state);
    pSwitch->set_halign(Gtk::Align::END);
    pSwitch->set_hexpand(false);
    pSwitch->set_valign(Gtk::Align::CENTER);
    pSwitch->add_css_class(pCssClass);
    pSwitch->property_active().signal_changed().connect([&state, &signal, pSwitch] {
        state= pSwitch->get_active();
        signal.emit(state);
    });
    pRow->append(*pSwitch);
    box.append(*pRow);
}
