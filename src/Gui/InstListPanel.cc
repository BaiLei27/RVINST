#include "Gui/GlibString.hh"
#include "Gui/InstListPanel.hh"
#include "ISA/InstFormat.hh"
#include "Util/InstCatalog.hh"

namespace {

// Display names / expander order. Indices are the slots in G_XLEN_SLOT.
constexpr std::array<std::string_view, 12> G_MODULE_ORDER {
    "RV32I",    // 0
    "RV64I",    // 1
    "RV128I",   // 2
    "Zifencei", // 3
    "Zicsr",    // 4
    "M",        // 5
    "A",        // 6
    "F",        // 7
    "D",        // 8
    "Q",        // 9
    "C",        // 10
    "Priv",     // 11
};

// XLEN_ tag → G_MODULE_ORDER slot (identity + RV*N* aliases). Exact match only.
constexpr std::array<std::pair<std::string_view, std::uint8_t>, 25> G_XLEN_SLOT {
    {
     { "RV32I", 0 },
     { "RV64I", 1 },
     { "RV128I", 2 },
     { "Zifencei", 3 },
     { "Zicsr", 4 },
     { "M", 5 },
     { "RV32M", 5 },
     { "RV64M", 5 },
     { "A", 6 },
     { "RV32A", 6 },
     { "RV64A", 6 },
     { "RV128A", 6 },
     { "F", 7 },
     { "RV32F", 7 },
     { "RV64F", 7 },
     { "D", 8 },
     { "RV32D", 8 },
     { "RV64D", 8 },
     { "Q", 9 },
     { "RV32Q", 9 },
     { "RV64Q", 9 },
     { "C", 10 },
     { "RV32C", 10 },
     { "RV64C", 10 },
     { "Priv", 11 },
     }
};

[[nodiscard]] constexpr size_t MODULE_SLOT(std::string_view xlen) noexcept
{
    for(const auto &[TAG, SLOT]: G_XLEN_SLOT) {
        if(TAG == xlen) return SLOT;
    }
    return G_MODULE_ORDER.size(); // unknown → discarded by caller
}

/**
 * @brief Pack a minimal sample word from the table key so Decode() can resolve the
 * mnemonic without a full assembly line.
 * SYSTEM: ecall/ebreak keep imm12 in PAY; Zicsr keys use (f3<<8) — put f3 in funct3.
 * C: rebuild a 16-bit encoding whose resolveDecodeKey matches the catalog key.
 */
uint32_t DefaultWord(const util::InstCatalog &entry) noexcept
{
    const uint32_t KEY= entry.functKey_;
    const uint32_t OPC= KEY & 0x7FU;
    const uint32_t PAY= KEY >> 7;
    uint32_t res {};

    switch(entry.format_) {
    case InstFormat::R: {
        switch(DetectRShape(OPC)) {                                                // Match RType PACK_KEY shapes (AMO/OP_FP/R4); classic OP uses funct7<<25.
        case RShape::AMO: res= (PAY >> 3) << 27 | (PAY & 0x7U) << 12 | OPC; break; // PAY = (funct5 << 3) | funct3 → funct5@[31:27], aq/rl left 0
        case RShape::FP:  {
            // PAY = (f7 << 8) | (rs2 << 3) | f3key, f7 = (funct5 << 2) | fmt
            const uint32_t F7 = PAY >> 8;
            const uint32_t RS2= (PAY >> 3) & 0x1FU;
            const uint32_t F3 = PAY & 0x7U;
            res               = F7 << 25 | RS2 << 20 | F3 << 12 | OPC;
        } break;
        case RShape::R4: res= (PAY & 0x3U) << 25 | 0x7U << 12 | OPC; break; // PAY = fmt; rm=0b111 (dyn) so the sample matches encode expects
        default:         res= (PAY >> 3) << 25 | (PAY & 0x7U) << 12 | OPC; break;
        }
    } break;

    case InstFormat::I: {
        if(OpcMap::SYSTEM == OPC) {
            res= PAY <= 1U ? PAY << 20 | OPC : (PAY >> 8 & 0x7U) << 12 | OPC; // ecall/ebreak: PAY is imm12 (0/1). Zicsr: PAY is f3<<8.
        } else if(PAY > 0x7U) {                                               // else-if: SYSTEM always owns res, must not fall through
            res= (PAY >> 3) << 25 | (PAY & 0x7U) << 12 | OPC;
        } else {
            res= (PAY & 0x7U) << 12 | OPC;
        }
    } break;

    case InstFormat::S:
    case InstFormat::B: res= (PAY & 0x7U) << 12 | OPC; break;
    case InstFormat::C: {
        const uint8_t Q  = PAY & 0x3U;
        const uint8_t F3 = PAY >> 8 & 0x7U;
        const uint8_t SUB= PAY >> 2 & 0x3FU;

        uint32_t c= Q | F3 << 13; // fits 16-bit range; no narrowing needed

        switch(Q) {
            // CIW/CL/CS: keep non-zero so BitField_.none() is false; F3=3/7 → sub=1 (LD/SD).
            // nzuimm ≠ 0 for c.addi4spn
        case 0U: c|= (0U == F3) ? (1U << 5) : (1U << 2 | 1U << 7); break;
        case 1U: {
            if(0U == F3 && 1U == SUB) { // c.nop: rd=0, imm=0

            } else if(3U == F3 && 1U == SUB) {
                c|= 2U << 7; // c.addi16sp (rd=x2)
                c|= 1U << 2;
            } else if(4U == F3) {
                switch(SUB) {
                case 8U: {
                    c|= 2U << 10;
                    c|= 1U << 2;
                } break; // c.andi

                case 0U: c|= 1U << 2; break;
                case 1U: {
                    c|= 1U << 12;
                    c|= 1U << 2;
                } break; // c.srai

                default: {
                    c|= (SUB >> 2) << 10;
                    c|= (SUB & 0x3U) << 5; // keep parens: & binds looser than <<
                    c|= 1U << 7 | 1U << 2;
                } break;
                }
            } else {
                c|= 1U << 7 | 1U << 2;
            }
            break;
        }
        case 2U: {
            if(4U == F3) {
                switch(SUB) {
                case 0U: c|= 1U << 7; break;                      // c.jr
                case 1U: c|= 1U << 12 | 1U << 7; break;           // c.jalr
                case 2U: c|= 1U << 12; break;                     // c.ebreak
                case 3U: c|= 1U << 12 | 1U << 7 | 1U << 2; break; // c.add
                case 4U: c|= 1U << 7 | 1U << 2; break;            // c.mv
                default: break;                                   // undefined encoding: leave c unchanged
                }
            } else {
                c|= 1U << 7 | 1U << 2;
            }
        } break;

        default: break; // Q == 3: undefined encoding, leave c unchanged
        }
        res= c;
    } break;

    default: res= OPC; break;
    }

    return res;
}

} // namespace

InstListPanel::InstListPanel(Gtk::Window &parent)
    : PParent_(&parent), PBtn_(Gtk::make_managed<Gtk::ToggleButton>())
{
    PBtn_->set_icon_name("view-list-symbolic");
    PBtn_->set_tooltip_text("Instruction list");
    PBtn_->add_css_class("icon-button");
    PBtn_->signal_toggled().connect(sigc::mem_fun(*this, &InstListPanel::onToggled));
}

Gtk::ToggleButton *InstListPanel::GetButton() const noexcept { return PBtn_; }

void InstListPanel::setButtonActive(bool active)
{
    if(!PBtn_ || PBtn_->get_active() == active) return;
    SyncingBtn_= true;
    PBtn_->set_active(active);
    SyncingBtn_= false;
}

void InstListPanel::onToggled()
{
    if(SyncingBtn_ || !PBtn_) return;
    if(PBtn_->get_active()) {
        showWindow();
    } else {
        hideWindow();
    }
}

void InstListPanel::showWindow()
{
    if(!PWindow_) build();
    const int PARENT_H= PParent_ ? std::max(PParent_->get_height(), 480) : 720;
    PWindow_->set_default_size(260, PARENT_H);
    PWindow_->present();
}

void InstListPanel::hideWindow()
{
    if(PWindow_) PWindow_->hide();
}

void InstListPanel::build()
{
    PWindow_= std::make_unique<Gtk::Window>();
    PWindow_->set_title("Instructions");
    PWindow_->set_transient_for(*PParent_);
    PWindow_->set_modal(false); // keep main window interactive
    PWindow_->set_resizable(true);
    PWindow_->set_hide_on_close(true);
    PWindow_->set_destroy_with_parent(true);
    PWindow_->add_css_class("inst-list-window");

    PWindow_->signal_hide().connect([this] { setButtonActive(false); });

    PPanel_= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    PPanel_->set_margin(12);
    PPanel_->add_css_class("inst-list-panel");

    auto *pTitle= Gtk::make_managed<Gtk::Label>("Instructions");
    pTitle->set_halign(Gtk::Align::START);
    pTitle->set_xalign(0.0F);
    pTitle->add_css_class("inst-list-title");
    PPanel_->append(*pTitle);

    PScroll_= Gtk::make_managed<Gtk::ScrolledWindow>();
    PScroll_->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    PScroll_->set_propagate_natural_width(true);
    PScroll_->set_propagate_natural_height(false);
    PScroll_->set_vexpand(true);
    PScroll_->set_hexpand(true);
    PScroll_->set_min_content_height(0);
    PScroll_->add_css_class("inst-list-scroll");

    auto *pModules= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    pModules->set_halign(Gtk::Align::FILL);
    pModules->add_css_class("inst-list-modules");

    std::array<std::vector<util::InstCatalog>, G_MODULE_ORDER.size()> byModule {};
    const auto ALL= util::GetAllInstructions();
    for(auto &bucket: byModule) {
        bucket.reserve(ALL.size() / G_MODULE_ORDER.size() + 8);
    }
    for(const auto &entry: ALL) {
        const auto SLOT= MODULE_SLOT(entry.xlen_);
        if(SLOT < byModule.size()) byModule[SLOT].push_back(entry);
    }

    for(size_t i= 0; i < G_MODULE_ORDER.size(); ++i) {
        const auto &entries= byModule[i];

        auto *pExpander= Gtk::make_managed<Gtk::Expander>(std::string { G_MODULE_ORDER[i] });
        pExpander->set_expanded("RV32I" == G_MODULE_ORDER[i]);
        pExpander->add_css_class("inst-list-module");
        if(entries.empty()) pExpander->add_css_class("inst-list-module-empty");

        auto *pList= Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
        pList->set_halign(Gtk::Align::FILL);
        pList->set_margin_top(6);
        pList->add_css_class("inst-list-rows");

        for(const auto &entry: entries) {
            const std::string_view NAME= entry.name_;
            const uint32_t WORD        = DefaultWord(entry);

            auto *pBtn= Gtk::make_managed<Gtk::Button>();
            pBtn->set_halign(Gtk::Align::FILL);
            pBtn->set_hexpand(true);
            pBtn->set_has_frame(false);
            pBtn->add_css_class("inst-list-row");

            auto *pLabel= Gtk::make_managed<Gtk::Label>(gui::ToGlibUstring(NAME));
            pLabel->set_halign(Gtk::Align::START);
            pLabel->set_hexpand(true);
            pLabel->set_xalign(0.0F);
            pLabel->add_css_class("inst-list-mnemonic");
            pBtn->set_child(*pLabel);

            pBtn->signal_clicked().connect([this, NAME, WORD] {
                onInstructionActivated(NAME, WORD);
            });
            pList->append(*pBtn);
        }

        pExpander->set_child(*pList);
        pModules->append(*pExpander);
    }

    PScroll_->set_child(*pModules);
    PPanel_->append(*PScroll_);
    PWindow_->set_child(*PPanel_);
}

void InstListPanel::onInstructionActivated(std::string_view name, uint32_t word)
{
    // Keep the list open for multi-pick; only the toggle / close hides it.
    signalInstruction_.emit(name, word);
}
