# Babeleo — KDE Plasma 6 Translator Applet

A KDE Plasma 6 panel applet for fast, one-click translation and dictionary lookups.
Select a word with the mouse, click the icon — your browser opens the result directly.

![Babeleo in action](https://github.com/tryptophane/babeleo-plasma/raw/main/screenshot.png)

---

## Features

- **One-click translation**: select text with the mouse, click the icon → browser opens with the result
- **Manual query**: enter a search term directly in the applet's popup dialog
- **Global keyboard shortcut**: configurable shortcut for hands-free use
- **Fully configurable search engines**: add, edit, remove any engine — the built-in list is just a starting point
- **Automatic favicon download**: engines display their website icon
- **Context menu**: quickly switch between engines, organize them into main menu and submenu

## Built-in search engines (all configurable)

The applet ships with a set of example engines to get you started. You can add,
remove or change all of them freely — none are mandatory:

- **leo.org** (EN↔DE, FR↔DE, ES↔DE, IT↔DE, ZH↔DE)
- **Wikipedia** (DE, EN, FR)
- **PONS** (DE↔EN, German monolingual)
- **Google Translate**
- **Duden**, **Real Academia Española**, **PubMed**, and more

No data is ever sent to any service without an explicit user action (clicking).
Your browser handles all communication — Babeleo only opens a URL.

---

## Requirements

| Dependency | Version |
|---|---|
| KDE Plasma | 6.x |
| Qt | ≥ 6.7 |
| KDE Frameworks | ≥ 6.0 |
| CMake | ≥ 3.22 |
| Extra CMake Modules (ECM) | ≥ 6.0 |

**Optional runtime dependency:**
- `wl-clipboard` — enables reading mouse-selected text on Wayland without pressing Ctrl+C.
  Without it, Babeleo falls back to the regular clipboard (Ctrl+C content).

---

## Build & Install

```bash
# Clone
git clone https://github.com/tryptophane/babeleo-plasma.git
cd babeleo-plasma

# Build
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
cmake --build .

# Install (requires root)
sudo cmake --install .
```

After installation, right-click the desktop or panel → *Add Widgets* → search for **Babeleo**.

To reload without restarting your session:
```bash
kquitapp6 plasmashell && kstart6 plasmashell
```

### Build dependencies

**Arch Linux / Manjaro:**
```bash
sudo pacman -S cmake extra-cmake-modules plasma-framework \
    qt6-base kf6-ki18n kf6-kio kf6-kcoreaddons kf6-kconfig \
    kf6-kglobalaccel kf6-kwidgetsaddons kf6-kxmlgui kf6-kwindowsystem
# Optional:
sudo pacman -S wl-clipboard
```

**KDE Neon / Ubuntu with Plasma 6:**
```bash
sudo apt install cmake extra-cmake-modules libplasma-dev qt6-base-dev \
    libkf6coreaddons-dev libkf6i18n-dev libkf6config-dev \
    libkf6globalaccel-dev libkf6kio-dev libkf6widgetsaddons-dev \
    libkf6xmlgui-dev libkf6windowsystem-dev
# Optional:
sudo apt install wl-clipboard
```

**Fedora:**
```bash
sudo dnf install cmake extra-cmake-modules plasma-devel qt6-qtbase-devel \
    kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kconfig-devel \
    kf6-kglobalaccel-devel kf6-kio-devel kf6-kwidgetsaddons-devel \
    kf6-kxmlgui-devel kf6-kwindowsystem-devel
# Optional:
sudo dnf install wl-clipboard
```

---

## License

GPL-2.0-or-later — see [COPYING](COPYING)

## Credits

- Icon based on *Crystal Clear app babelfish* by Everaldo Coelho (LGPL)
- Original KDE 4 concept by Pascal Pollet (2009)
- Ported to KDE Plasma 6 (2025)
