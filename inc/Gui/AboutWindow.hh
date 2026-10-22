#pragma once

#include <gtkmm.h>

#include <memory>

/**
 * @brief Lazily builds and presents the modal "About RVINST" window.
 *
 * Owns the window so it survives across shows. Builds the info card only on
 * first use; afterwards just re-presents the cached window.
 */
class AboutWindow {
public:
    explicit AboutWindow(Gtk::Window &parent);

    void Show();

private:
    void build();

    Gtk::Window *PParent_ {}; // non-owning; cppcoreguidelines forbids ref members
    std::unique_ptr<Gtk::Window> PWindow_;
};

// Date:26/10/19
