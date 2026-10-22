#pragma once

#include <gtkmm.h>
#include <sigc++/signal.h>

#include <functional>

#include "Core/Instruction.hh"
#include "Util/FileIo.hh"

/**
 * @brief Owns the "open file" button, the async Gtk::FileDialog and the batch
 *        line processor.
 *
 * Line-level events use sigc::signal (copyable args, multicast-friendly).
 * The decoded Instruction is handed off via OnInstruction_ instead:
 *
 *   - unique_ptr = exclusive ownership transfer to the window (single consumer).
 *   - sigc::signal cannot carry unique_ptr: emission copies/forwards to every
 *     slot, and a move-only type would leave remaining slots with an empty ptr.
 *   - Instruction & would keep ownership in FileLoader; the window could only
 *     observe, not take, the object.
 *   - shared_ptr would work with sigc but weakens the "window owns the current
 *     instruction" model for no multicast benefit here.
 */
class FileLoader {
private:
    Glib::RefPtr<Gtk::FileDialog> Dialog_;
    std::function<bool()> AbiOnFn_;
    std::function<void(std::unique_ptr<Instruction>)> OnInstruction_;
    Gtk::Window *POwner_ {};
    const util::FileIo *PFileIo_ {};
    Gtk::Button *PBtn_ {};

public:
    sigc::signal<void(std::string_view)> signalInput_;
    sigc::signal<void(std::string)> signalError_;

public:
    explicit FileLoader(Gtk::Window &owner,
                        const util::FileIo &fileIo,
                        std::function<bool()> abiOnFn);

    [[nodiscard]] Gtk::Button *GetButton() const noexcept;
    void SetOnInstruction(std::function<void(std::unique_ptr<Instruction>)> cb);

private:
    void onOpenClicked();
    void processLines(std::string_view path);
};

// Date:26/10/19
