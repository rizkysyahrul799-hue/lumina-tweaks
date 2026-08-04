<p align="center">
  <a href="https://github.com/rizkysyahrul799-hue/lumina-tweaks" target="_blank">
    <img src="https://raw.githubusercontent.com/rizkysyahrul799-hue/lumina-tweaks/main/assets/logo.png" width="200" alt="LUMina Tweaks Logo">
  </a>
</p>

<p align="center">
  <a href="https://t.me/MallArchiven"><img src="https://img.shields.io/badge/Follow-Telegram-white.svg?style=for-the-badge&logo=telegram&logoColor=white&labelColor=222" alt="Join Our Telegram Channel"></a>
  <a href="https://github.com/rizkysyahrul799-hue/lumina-tweaks/releases"><img src="https://img.shields.io/github/downloads/rizkysyahrul799-hue/lumina-tweaks/total?style=for-the-badge&logoColor=white&labelColor=222" alt="Total Downloads"></a>
  <a href="https://github.com/rizkysyahrul799-hue/lumina-tweaks/releases"><img src="https://img.shields.io/github/v/release/rizkysyahrul799-hue/lumina-tweaks?label=Release&style=for-the-badge&logo=github&logoColor=white&labelColor=222" alt="Latest Stable Version"></a>
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/Apache--2.0-white?style=for-the-badge&logo=apache&logoColor=white&label=License&labelColor=222" alt="License"></a>
</p>

## About LUMina Tweaks

**LUMina Tweaks** is an automated C++ native daemon module designed to dynamically optimize Android performance. Developed by **[@kimalaja](https://t.me/MallArchiven)** and powered by the native `luminad` engine, it intelligently balances CPU governor profiles, I/O schedulers, refresh rates, and thermal throttles based on active foreground applications.

---

## Key Features

- **Automated C++ Native Engine (`luminad`)**: Lightweight native binary with low resource consumption and real-time foreground app tracking.
- **Dynamic Profile Switcher**: Automatically toggles between **Performance**, **Balanced**, and **Eco** profiles based on your app list.
- **Thermal Unchained**: Bypasses aggressive vendor/hardware thermal throttling during gaming sessions.
- **Refresh Rate & Render Engine Tuning**: Dynamically force display refresh rates (up to max supported Hz) and switch backend renderers (Vulkan/OpenGL/Skia).
- **SoC Specific Optimizations**: Integrated MediaTek GPU boost and Qualcomm kernel tweaks.

---

## Supported Root Managers

- [KernelSU](https://github.com/tiann/KernelSU)
- [APatch](https://github.com/bmax121/APatch)
- [Magisk](https://github.com/topjohnwu/Magisk) <sup>(Daemon Service)</sup>

### WebUI Extensions Supported

- [KsuWebUI](https://github.com/5ec1cff/KsuWebUIStandalone) <sup>🌐</sup>
- [WebUI-X](https://github.com/MMRLApp/WebUI-X-Portable) <sup>🌐</sup>
- [MMRL](https://github.com/MMRLApp/MMRL)

---

## Configuration

The core configuration files are located at `/data/adb/.config/lumina/`:

- `config.json`: Manages global system tweak toggles, refresh rates, and render engines.
- `gamelist.json`: Maps individual app package names to targeted performance profiles.

You can customize all settings directly via the **WebUI** or by editing the JSON files using root permissions.

---

## Community & Support

- [Telegram Channel](https://t.me/MallArchiven) - Latest news and release announcements
- [Report Issues](https://github.com/rizkysyahrul799-hue/lumina-tweaks/issues) - Found a bug? Submit an issue here
- [Contribute](https://github.com/rizkysyahrul799-hue/lumina-tweaks/pulls) - Pull requests are welcome!

---

## Credits & Special Thanks

Special thanks to the open-source community and developers whose work inspired and contributed to **LUMina Tweaks**:

- **[@kimalaja](https://t.me/MallArchiven)** - Project Lead & Developer
- **[Rem01Gaming](https://github.com/Rem01Gaming)** - For the WebUI Vue.js base framework and project inspiration.
- **[Liliya2727](https://github.com/Liliya2727)** - For code references and contributions.
- **[Zexshia](https://github.com/Zexshia)** - For code references, tweak logic, and feature implementation ideas.
- **[KernelSU](https://github.com/tiann/KernelSU)**, **[APatch](https://github.com/bmax121/APatch)**, & **[Magisk](https://github.com/topjohnwu/Magisk)** - For the Android root framework ecosystem.

---

## License

LUMina Tweaks is open-source software licensed under the [Apache-2.0 License](https://www.apache.org/licenses/LICENSE-2.0).
