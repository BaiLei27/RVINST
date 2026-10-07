#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <gtkmm.h>

#include "Gui/IHoverWidget.hh"
#include "Gui/InstCommST.hh"
#include "ISA/InstFormat.hh"
#include "Util/InstFormatView.hpp"

class AsmMnemonicWidget;

class BinaryFieldWidget: public IHoverWidget {
public:
    std::vector<Gtk::Label *> controlLabels_;
    std::vector<BinaryFieldWidget *> relatedBinary_;
    std::vector<AsmMnemonicWidget *> relatedAsm_;

public:
    BinaryFieldWidget(const InstField &field, int &nibbleIndex);

public:
    [[nodiscard]] const std::vector<Gtk::Label *> &GetLabels() const noexcept { return controlLabels_; }

    void SetupHover(std::string_view name,
                    const util::InstFormatView &view,
                    InstCommST::BinaryFieldWidgetMap_u &binaryFieldWidgets,
                    InstCommST::AsmMnemonicWidgetMap_u &asmFieldWidgets);
    void UpdateBits(uint32_t fieldValue);

protected:
    void onHoverChanged(bool on) override;
};

// Date:26/10/07/21:58
