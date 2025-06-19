// hosts_window.hpp – TUI widget for listing / adding hosts
// -----------------------------------------------------------------------------
#pragma once

// Dear ImGui
#include <imgui.h>

#include <optional>
#include <string>
#include <vector>

#include "host_service.hpp" // service façade for daemon access

/**
 * HostsWindow — immediate‑mode widget that shows the Hosts / Nodes table and an
 * “Add Host” modal.  Keep UI‑only state inside this class; all persistence and
 * daemon I/O is delegated to HostService.
 */
class HostsWindow
{
public:
    explicit HostsWindow(HostService &service) noexcept : svc_{service} {}

    /**
     * Draw the window. Call once per frame from the main event‑loop. The
     * `open` flag is owned by the caller (so the caller decides whether the
     * window should appear at all — e.g. toggled from a menu item).
     */
    inline void draw(bool *open)
    {
        if (!open || !*open)
            return;

        // --- Window placement (once) --------------------------------------------------
        ImGui::SetNextWindowPos({0, 1}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({150.f, 20.f}, ImGuiCond_Once);

        if (!ImGui::Begin("Hosts / Nodes", open, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return; // window was collapsed -> nothing to do this frame
        }

        // --- Filter box --------------------------------------------------------------
        if (ImGui::InputTextWithHint("##filter", "filter by name / host…",
                                     filterBuf_, sizeof(filterBuf_)))
        {
            dirtyFilter_ = true;
        }

        ImGui::Separator();
        drawTable_();

        // --- Add host button ---------------------------------------------------------
        if (ImGui::Button("Add Host"))
        {
            draft_.emplace(); // default‑construct fresh descriptor
            modalOpen_ = true;
            ImGui::OpenPopup("AddHostPopup");
        }

        drawAddHostModal_();

        ImGui::End();
    }

private:
    // -----------------------------------------------------------------------------
    // Child helpers — these keep the draw() method small and readable
    // -----------------------------------------------------------------------------

    inline void drawTable_()
    {
        // Fetch (and cache) hosts each frame.  The service might be polling the
        // daemon in the background or caching internally; here we just call its
        // accessor.
        const auto &hosts = svc_.listHosts();

        // Simple client‑side filter on name or host string -----------------------
        auto matchesFilter = [&](const ledgr::HostDescriptor &h)
        {
            if (!dirtyFilter_)
                return true; // no filter text entered
            const std::string needle{filterBuf_};
            return h.name.find(needle) != std::string::npos ||
                   h.host.find(needle) != std::string::npos;
        };

        if (ImGui::BeginTable("HostLedger", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Host");
            ImGui::TableHeadersRow();

            for (const auto &h : hosts)
            {
                if (!matchesFilter(h))
                    continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(h.id.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(h.name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(h.host.c_str());
            }
            ImGui::EndTable();
        }
    }

    inline void drawAddHostModal_()
    {
        if (!modalOpen_)
            return; // no popup scheduled

        if (ImGui::BeginPopupModal("AddHostPopup", &modalOpen_))
        {
            auto &d = *draft_; // safe: draft_ is engaged when modalOpen_ is true

            ImGui::InputText("ID", &d.id);
            ImGui::InputText("Name", &d.name);
            ImGui::InputText("Host", &d.host);

            bool ok = ImGui::Button("Add");
            ImGui::SameLine();
            bool canc = ImGui::Button("Cancel");

            if (ok)
            {
                auto res = svc_.addHost(d);
                if (res)
                {
                    modalOpen_ = false; // close on success
                }
                else
                {
                    // Show error tooltip next frame (simplest UX)
                    errorMessage_ = std::move(res.error());
                }
            }
            else if (canc)
            {
                modalOpen_ = false;
            }

            if (!errorMessage_.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 80, 80, 255));
                ImGui::TextUnformatted(errorMessage_.c_str());
                ImGui::PopStyleColor();
            }

            if (!modalOpen_)
            {
                draft_.reset();
                errorMessage_.clear();
            }

            ImGui::EndPopup();
        }
    }

private:
    HostService &svc_;

    // Transient UI state ----------------------------------------------------------
    std::optional<ledgr::HostDescriptor> draft_{}; //!< form under construction
    bool modalOpen_{false};
    bool dirtyFilter_{false};
    char filterBuf_[64]{};     //!< small fixed buffer is fine here
    std::string errorMessage_; //!< last add‑host error (if any)
};
