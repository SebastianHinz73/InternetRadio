# Lyrat Radio

With this source, Espressif's Lyrat Board becomes an Internet radio.

## How program

To build the program the **[ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)** and the **[ESP-ADF](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/)** must be installed.
Setting up the build environment is well described there.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p COM3 flash monitor
```
