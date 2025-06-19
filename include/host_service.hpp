#pragma once

#include <vector>
#include <shared_mutex>
#include <string>
#include <expected>
#include <mutex>

// #include <spdlog/spdlog.h>

#include "host_descriptor.hpp"
#include "api_client.hpp"

/**
 * HostService
 * ---------------------------
 * Owns the already‑connected `LedgerApiClient`, keeps a cached copy of the
 * host list, and translates daemon errors into `std::expected` so the UI can
 * treat them as ordinary values.  Thread‑safe via a `std::shared_mutex`.
 */
class HostService
{
public:
    explicit HostService(LedgerApiClient &client) noexcept : client_{client}
    {
        refresh_(); // prime cache
    }

    /** Return the cached hosts.  Never throws. */
    const std::vector<ledgr::HostDescriptor> &listHosts() const noexcept
    {
        std::shared_lock lock{mtx_};
        return cache_;
    }

    /**
     * Add a host via the daemon.  Success ⇒ cache updated; error ⇒ returned as
     * `std::unexpected<string>`.
     */
    [[nodiscard]] std::expected<void, std::string>
    addHost(const ledgr::HostDescriptor &h) noexcept
    {
        std::string err;
        if (client_.add_host(h, &err))
        {
            {
                std::unique_lock lock{mtx_};
                cache_.push_back(h);
            }
            return {};
        }
        return std::unexpected(err);
    }

    /** Refresh the cache from the daemon.  Non‑throwing; logs on failure. */
    void refresh() noexcept { refresh_(); }

private:
    void refresh_() noexcept
    {
        try
        {
            auto hosts = client_.list_hosts();
            std::unique_lock lock{mtx_};
            cache_ = std::move(hosts);
        }
        catch (const std::exception &ex)
        {
            // spdlog::warn("HostService refresh failed: {}", ex.what());
        }
    }

    LedgerApiClient &client_;
    std::vector<ledgr::HostDescriptor> cache_;
    mutable std::shared_mutex mtx_;
};
