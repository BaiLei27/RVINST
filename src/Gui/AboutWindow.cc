#include <format>
#include <string_view>

#include "Gui/AboutWindow.hh"
#include "Gui/GlibString.hh"
#include "config.hpp"

AboutWindow::AboutWindow(Gtk::Window &parent): PParent_(&parent) { }

void AboutWindow::Show()
{
    if(!PWindow_) build();
    PWindow_->present();
}

void AboutWindow::build()
{
    PWindow_= std::make_unique<Gtk::Window>();
    PWindow_->set_title("About RVINST");
    PWindow_->set_transient_for(*PParent_);
    PWindow_->set_modal(true);
    PWindow_->set_resizable(false);
    PWindow_->set_hide_on_close(true);
    PWindow_->add_css_class("about-window");

    auto *pBox= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
    pBox->set_margin(24);
    pBox->set_halign(Gtk::Align::FILL);
    pBox->add_css_class("about-container");

    auto *pTitle= Gtk::make_managed<Gtk::Label>("RVINST");
    pTitle->add_css_class("version-title");
    pTitle->set_halign(Gtk::Align::START);
    pTitle->set_xalign(0.0F);
    pTitle->set_justify(Gtk::Justification::LEFT);
    pBox->append(*pTitle);

    auto *pInfoCard= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    pInfoCard->set_halign(Gtk::Align::FILL);
    pInfoCard->add_css_class("version-info-card");

    auto appendInfoRow= [pInfoCard](std::string_view name, std::string_view value, bool link= false) {
        auto *pRow= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
        pRow->set_halign(Gtk::Align::FILL);
        pRow->add_css_class("version-info-row");

        auto *pName= Gtk::make_managed<Gtk::Label>(gui::ToGlibUstring(name));
        pName->set_halign(Gtk::Align::START);
        pName->set_valign(Gtk::Align::CENTER);
        pName->set_xalign(0.0F);
        pName->set_width_chars(10);
        pName->add_css_class("version-info-name");

        auto *pValue= Gtk::make_managed<Gtk::Label>();
        pValue->set_halign(Gtk::Align::START);
        pValue->set_valign(Gtk::Align::CENTER);
        pValue->set_hexpand(true);
        pValue->set_selectable(true);
        pValue->set_can_focus(false);
        pValue->set_xalign(0.0F);
        pValue->add_css_class("version-info-value");
        if(link) {
            const Glib::ustring URI= gui::ToGlibUstring(value);

            const auto MARKUP= std::format("<a href=\"{}\">{}</a>",
                                           Glib::Markup::escape_text(URI).raw(),
                                           Glib::Markup::escape_text(URI).raw());
            pValue->set_markup(MARKUP);
            pValue->set_tooltip_text(URI);
            pValue->add_css_class("version-info-link");
            pValue->signal_activate_link().connect(
                [](const Glib::ustring &uri) -> bool {
                    Gio::AppInfo::launch_default_for_uri(uri);
                    return true;
                },
                false);
        } else {
            pValue->set_text(gui::ToGlibUstring(value));
        }

        pRow->append(*pName);
        pRow->append(*pValue);
        pInfoCard->append(*pRow);
    };

    appendInfoRow("Version:", util::info::G_PROJECT_VERSION);
    appendInfoRow("Commit:", util::info::G_COMMIT_HASH);
    appendInfoRow("URL:", util::info::G_HOMEPAGE_URL, true);
    appendInfoRow("Copyright:", util::info::G_COPYRIGHT);
    appendInfoRow("GTKMM:", std::format("{}.{}.{}", GTKMM_MAJOR_VERSION, GTKMM_MINOR_VERSION, GTKMM_MICRO_VERSION));
    appendInfoRow("GTK:", std::format("{}.{}.{}", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION));
    appendInfoRow("GLib:", std::format("{}.{}.{}", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION));

    pBox->append(*pInfoCard);

    auto *pCloseBtn= Gtk::make_managed<Gtk::Button>("Close");
    pCloseBtn->set_halign(Gtk::Align::END);
    pCloseBtn->add_css_class("about-close-button");
    pCloseBtn->signal_clicked().connect([this] { PWindow_->hide(); });
    pBox->append(*pCloseBtn);

    PWindow_->set_child(*pBox);
}
