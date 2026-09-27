Contents of this folder go to the LittleFS partition with

    pio run -t uploadfs

Put here (never commit them -- both are in .gitignore):

  bally.key   from BTPDevice's tools/provision_key.py; imported into NVS on
              the first boot and renamed to bally.key.imported. Needed by the
              radio links (TCP, BLE); the USB link works without it.
  wifi.txt    one "ssid,password" per line, for the TCP link
