#include "Gui/RISCVInstWindow.hh"

int main(int argc, char *argv[])
{
    auto app= Gtk::Application::create("org.gtkmm.riscv.visualization");
    std::unique_ptr<RISCVInstWindow> window;

    app->signal_activate().connect([&] {
        if(window) {
            window->present();
            return;
        }

        window= std::make_unique<RISCVInstWindow>();
        app->add_window(*window);
        window->signal_close_request().connect(
            [&] {
                window.reset();
                return true;
            },
            false);
        window->show();
    });

    return app->run(argc, argv);
}
