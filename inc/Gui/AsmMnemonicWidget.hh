
#pragma once

#include <gtkmm.h>
#include <string_view>
#include <vector>

#include "Gui/IHoverWidget.hh"
#include "Gui/InstCommST.hh"
#include "Util/InstFormatView.hpp"

class BinaryFieldWidget;

class AsmMnemonicWidget: public IHoverWidget {
public:
    Gtk::Label *pLabel_ {};
    std::vector<BinaryFieldWidget *> relatedBinary_;

    AsmMnemonicWidget(std::string_view token, Gtk::Box *pParentAsmBox);

public:
    [[nodiscard]] Gtk::Label *GetLabel() const noexcept;

    void SetupHover(std::string_view name,
                    const util::InstFormatView &view,
                    InstCommST::BinaryFieldWidgetMap_u &binaryFieldWidgets);

protected:
    void onHoverChanged(bool on) override;
};

// Date:26/10/07/21:58
