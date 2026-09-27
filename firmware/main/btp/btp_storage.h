// Where the device keeps its files: LittleFS on the "storage" partition
// (partitions.csv), filled from data/ with `pio run -t uploadfs` -- the key
// (bally.key) and the Wi-Fi networks (wifi.txt) for the radio links.
#pragma once
#include <btpdev/config.h>

inline const btpdev::StorageConfig storage_config = {};
