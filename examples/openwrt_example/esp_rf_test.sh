#!/bin/sh

wait_for_key() {
  local prompt="$1"
  local char
  
  [ -n "$prompt" ] && echo -n "$prompt"
  
  while true; do
    char=$(dd if=/dev/ttyS2 bs=1 count=1 2>/dev/null)
    
    # Skip NULL (0x00)
    [ -z "$char" ] && continue
    
    # Compare to original characters, not their ASCII codes, for better readability
    case "$char" in
      $'\n'|$'\r')  # Newline (Enter key) and Carriage Return (Return key)
        echo
        return 0
        ;;
      ' ')          # Space key fixed at ASCII 32
        echo " "
        return 0
        ;;
    esac
  done
}
############ ESP32-RF test program ############

# To flash ESP32-RFTest bin by esp_flasher. Stop SDIO driver first.
rmmod mtk-sd

./esp_flasher -r ./ESP32-C6_RFTest_V106_d12e5a3_20250711.bin

sleep 1

# To send command to /dev/ttyS1 for test steps and each step need to wait user to press Enter key to continue.

# Wi-Fi TX channel 1 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "cbw40m_en 0\r\ntx_contin_en 1\r\nphy_11ax_tx_set 0 16 1 61\r\nesp_tx 1 23 0 75000\r\n" > /dev/ttyS1
wait_for_key "Wi-Fi TX channel 1 start. Press Enter to continue... "

# Wi-Fi TX channel 5 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "cbw40m_en 0\r\ntx_contin_en 1\r\nphy_11ax_tx_set 0 16 1 61\r\nesp_tx 5 23 0 75000\r\n" > /dev/ttyS1
wait_for_key "Wi-Fi TX channel 5 start. Press Enter to continue... "

# Wi-Fi TX channel 11 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "cbw40m_en 0\r\ntx_contin_en 1\r\nphy_11ax_tx_set 0 16 1 61\r\nesp_tx 11 23 0 75000\r\n" > /dev/ttyS1
wait_for_key "Wi-Fi TX channel 11 start. Press Enter to continue... "

# BLE TX channel 37 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "fcc_le_tx 15 0 250 2 0 0 1\r\n" > /dev/ttyS1
wait_for_key "BLE TX channel 37 start. Press Enter to continue... "

# BLE TX channel 18 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "fcc_le_tx 15 20 250 2 0 0 1\r\n" > /dev/ttyS1
wait_for_key "BLE TX channel 18 start. Press Enter to continue... "

# BLE TX channel 39 start:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1
echo -e "fcc_le_tx 15 39 250 2 0 0 1\r\n" > /dev/ttyS1
wait_for_key "BLE TX channel 39 start. Press Enter to continue... "

# Stop all test item:
echo -e "cmdstop\r\ntx_contin_en 0\r\n" > /dev/ttyS1

# To restart SDIO driver
insmod mtk-sd
