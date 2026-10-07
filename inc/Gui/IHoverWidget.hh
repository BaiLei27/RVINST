#pragma once

#include <gtkmm.h>
#include <span>
#include <string_view>
#include <vector>

class IHoverWidget {
protected:
    Gtk::Widget *PRoot_ {};
    Glib::RefPtr<Gtk::EventControllerMotion> PMotion_;
    bool Hover_ {};

public:
    IHoverWidget()                                = default;
    IHoverWidget(const IHoverWidget &)            = delete;
    IHoverWidget &operator= (const IHoverWidget &)= delete;
    virtual ~IHoverWidget()                       = default;

public:
    void Highlight();
    void Unhighlight();

    [[nodiscard]] Gtk::Widget *GetRoot() const noexcept;

    static void SetCSSClass(Gtk::Widget *pWidget, const char *pCls, bool on) noexcept;
    static void SetHighlighted(IHoverWidget *pWidget, bool on) noexcept;

    template <class W>
    static void SetHighlightedAll(const std::vector<W *> &widgets, bool on) noexcept
    {
        for(W *pWidget: widgets) {
            SetHighlighted(pWidget, on);
        }
    }

    template <class Map>
    static void CollectRelated(Map &map,
                               std::span<const std::string_view> keys,
                               std::vector<typename Map::mapped_type> &out,
                               typename Map::mapped_type except= nullptr)
    {
        out.clear();
        out.reserve(keys.size());
        for(const std::string_view &k: keys) {
            auto it= map.find(k);
            if(it == map.end() || it->second == except) continue;

            out.emplace_back(it->second);
        }
    }

protected:
    void attachMotion();
    void applyHover(bool on);

    virtual void onHoverChanged(bool on)= 0;
};
