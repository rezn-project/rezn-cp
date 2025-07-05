#ifndef CP_STATS_WINDOW_HPP
#define CP_STATS_WINDOW_HPP
#include <inttypes.h>
#include <imgui.h>
#include <string>
#include <map>
#include "readerwriterqueue.h"
#include "stats_model.hpp"

class StatsWindow
{
public:
    explicit StatsWindow(moodycamel::ReaderWriterQueue<StatsMap> *q) noexcept
        : queue_(q) {}

    void draw(bool *open)
    {
        if (!open || !*open)
            return;

        ImGui::SetNextWindowPos({0, 1}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({150.f, 20.f}, ImGuiCond_Once);

        if (!ImGui::Begin("Stats", open, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        pumpQueue(); // <-- merge fresh data into ledger_
        drawTable_();

        ImGui::End();
    }

private:
    // merge every batch available this frame
    void pumpQueue()
    {
        StatsMap batch;
        while (queue_->try_dequeue(batch))
        { // drains queue
            for (auto &[id, ts] : batch)
                ledger_[id] = std::move(ts); // overwrite newest
        }
    }

    void drawTable_()
    {
        if (!ImGui::BeginTable("HostLedger", 3,
                               ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
            return;

        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("CPU");
        ImGui::TableSetupColumn("Memory");
        ImGui::TableHeadersRow();

        for (auto &[id, ts] : ledger_)
        {
            double cpu = ts.stats.cpu_avg.value_or(0.0);
            uint64_t mem = ts.stats.max_mem.value_or(0);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(id.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f %%", cpu * 100.0);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%" PRIu64 " KiB", mem / 1024);
        }
        ImGui::EndTable();
    }

    moodycamel::ReaderWriterQueue<StatsMap> *queue_;
    std::map<std::string, TimestampedStats> ledger_; // persistent
};
#endif
