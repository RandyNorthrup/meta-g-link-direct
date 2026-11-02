// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/wifi_station.hpp"

#include <algorithm>
#include <string>

namespace mgl {
bool wifiAdapterPresent(const std::string& adapter_id) {
    const auto adapters = listWifiAdapters();
    return std::ranges::any_of(
        adapters, [&](const WifiAdapter& adapter) { return adapter.id == adapter_id; });
}

#ifndef __APPLE__
std::string wifiNetworkName(const std::string& fresh) { return fresh; }
#endif
} // namespace mgl
