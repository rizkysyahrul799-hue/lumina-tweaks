# LUMina Tweaks

**LUMina Tweaks** is an Android module powered by a native C++ daemon (`luminad`), designed to be **simple and straightforward without complex configurations**. It automatically manages system profiles, CPU governors, and refresh rates based on the foreground application and screen state.

---

## Key Features

- **Simple & Plug-and-Play**: Works automatically out-of-the-box without confusing settings.
- **Native C++ Engine (`luminad`)**: Highly efficient (*low-overhead*) with real-time application tracking.
- **Smart Screen-Off Logic**: Automatically switches to energy-saving mode when the screen turns off.
- **Dynamic Profile Switcher**: Seamlessly adjusts CPU profiles based on the active app.
- **Refresh Rate Manager**: Dynamic display refresh rate adjustments.

---

## How Profiles Work

The module manages device performance through 3 main profiles:

- **Performance**  
  Maximizes CPU clusters and frequencies to ensure peak performance for gaming or heavy apps without frame drops.
- **Balanced**  
  Maintains an optimal balance between system responsiveness and battery efficiency for daily tasks (social media, messaging, browsing).
- **Eco (Standby & Screen-Off)**  
  Scales down CPU frequencies and clusters to their lowest efficient states. Automatically triggers when the **screen turns off** to preserve battery life, returning to normal once the screen wakes up.

---

## Root Support

- **KernelSU**
- **APatch**
- **Magisk**

---

## Configuration

All configuration files are located at `/data/adb/.config/lumina/`:

- `config.json` — Manages global tweak toggles and refresh rates.
- `gamelist.json` — Maps app package names to their respective performance profiles.

Settings can be changed via **WebUI** or edited manually using the JSON files.

---

## Community & Credits

- **Developer**: [@kimalaja](https://t.me/MallArchiven)
- **Special Thanks**: [Rem01Gaming](https://github.com/Rem01Gaming) (Base WebUI), [Liliya2727](https://github.com/Liliya2727) (Code & Logic References)
- **Repository**: [lumina-tweaks](https://github.com/rizkysyahrul799-hue/lumina-tweaks)

---

## License

This project is licensed under the [Apache License 2.0](LICENSE).
