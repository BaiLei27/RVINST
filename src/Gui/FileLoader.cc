
#include <format>

#include "Util/InputParse.hh"
#include "Gui/FileLoader.hh"

FileLoader::FileLoader(Gtk::Window &owner,
                       const util::FileIo &fileIo,
                       std::function<bool()> abiOnFn)
    : POwner_(&owner),
      PFileIo_(&fileIo),
      PBtn_(Gtk::make_managed<Gtk::Button>()),
      AbiOnFn_(std::move(abiOnFn))
{

    PBtn_->set_icon_name("document-open-symbolic");
    PBtn_->set_tooltip_text("Open input file");
    PBtn_->add_css_class("icon-button");
    PBtn_->signal_clicked().connect(sigc::mem_fun(*this, &FileLoader::onOpenClicked));
}

Gtk::Button *FileLoader::GetButton() const noexcept { return PBtn_; }

void FileLoader::SetOnInstruction(std::function<void(std::unique_ptr<Instruction>)> cb)
{
    OnInstruction_= std::move(cb);
}

void FileLoader::onOpenClicked()
{
    // Keep the dialog alive for the async callback: a plain local would be
    // destroyed when this handler returns. Reuse the member if one is open.
    if(!Dialog_) {
        Dialog_= Gtk::FileDialog::create();
        Dialog_->set_title("Open input file");

        auto pFilter= Gtk::FileFilter::create();
        pFilter->set_name("Text and data files");
        pFilter->add_pattern("*.txt");
        pFilter->add_pattern("*.log");
        pFilter->add_pattern("*.data");
        pFilter->add_pattern("*");
        auto pFilters= Gio::ListStore<Gtk::FileFilter>::create();
        pFilters->append(pFilter);
        Dialog_->set_filters(pFilters);
    }

    Dialog_->open(*POwner_, [this](const Glib::RefPtr<Gio::AsyncResult> &result) {
        try {
            auto file= Dialog_->open_finish(result);
            if(file) processLines(file->get_path());
        } catch(const Gtk::DialogError &err) {
            if(Gtk::DialogError::Code::DISMISSED != err.code()) {
                signalError_.emit(std::format("file open failed: {}", err.what()));
            }
        } catch(const Glib::Error &err) {
            signalError_.emit(std::format("file open failed: {}", err.what()));
        }
        Dialog_.reset(); // release after the callback runs
    });
}

void FileLoader::processLines(std::string_view path)
{
    std::unique_ptr<Instruction> last;
    auto streamed= util::FileIo::ForEachLine(path, [this, &last](std::string_view raw) {
        const auto TRIMMED= util::TrimAsciiWs(raw);
        if(TRIMMED.empty()) return;

        auto parsed= util::ClassifyInstInput(TRIMMED);
        if(!parsed) {
            signalError_.emit(std::format("invalid input: {}", parsed.error().Msg()));
            return;
        }

        auto inst= util::MakeDecodedInstruction(TRIMMED, *parsed, AbiOnFn_());
        if(!inst) {
            signalError_.emit(std::format("decode failed: {}", inst.error().Msg()));
            return;
        }

        signalInput_.emit(TRIMMED);
        last= std::move(*inst);
    });
    if(!streamed) {
        signalError_.emit(std::format("file read failed: {}", streamed.error().Msg()));
        return;
    }

    if(last) {
        if(OnInstruction_) OnInstruction_(std::move(last));
    } else {
        signalError_.emit("no valid instructions found in file");
    }
}
