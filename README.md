# 🐾 Meow Browser

**Meow** is a lightweight, privacy-oriented desktop web browser built with **C++17** and **Qt6 (`QtWebEngine`)**. Designed for researchers, digital archivists, and power users, Meow goes beyond standard web browsing by offering deep integration with downloaded media metadata, intelligent full-page离线 archiving, custom network redirect handling, and built-in multi-layer ad blocking.

---
<img width="1842" height="1036" alt="image" src="https://github.com/user-attachments/assets/575725d6-a234-447b-ad53-333004b69625" />
---

## ✨ Key & Unique Features

### 1. 🏷️ Automatic EXIF Metadata Injection for Downloaded Images
When you download an image in Meow, the browser doesn't just save a raw file whose origin you will forget tomorrow. Using native GDI+ image encoders on Windows, Meow automatically embeds contextual provenance straight into the image's **EXIF properties**:
* **`TagXPKeywords` (`0x9c9e`):** Automatically populated with sanitized tags derived from the **page title**, the image's HTML **`alt` attribute** (extracted directly from the DOM element under your cursor), the **page host**, and the **image CDN host**.
* **`TagExifUserComment` (`0x9286`):** Stores the direct **source URL of the image** alongside the **URL of the parent webpage** where it was found.
* All metadata is immediately searchable and visible in standard OS file managers (e.g., Windows Explorer *Details* pane).

### 2. 📦 Smart Full-Page Offline Archiver (`Save Page`)
Unlike standard browsers that dump a messy folder or a brittle `.mhtml` snapshot, Meow's custom `FileManager` performs a structured, element-by-element local mirror of the active DOM:
* **Deep DOM & CSS Extraction:** Scans `<img>`, `<script>`, `<link>`, `<a>`, `<iframe>`, inline `style` attributes on `<div>`/`<span>`/`<a>`, and `<style>` blocks (parsing `url(...)` background images).
* **Local Directory Hierarchy:** Downloads every external asset via Meow's authenticated `DownloadManager`, preserving relative server paths inside your target folder and resolving naming collisions automatically.
* **DOM Rewriting:** Rewrites all asset references on a cloned DOM tree to relative local file paths and outputs a clean, self-contained `.html` file—then immediately opens your local copy in a new tab for verification.

### 3. 🕵️ Intelligent Download Manager & Soft-Redirect Resolver
Many modern file hosts and image boards serve an intermediate HTML landing page or a JavaScript-driven redirect when a download link is requested.
* **MIME & Magic-Byte Detection:** Automatically inspects `Content-Type` headers, maps over 60+ web MIME types, and falls back to `QImageReader` magic-byte inspection or HTML signature detection to ensure every saved file gets its true extension.
* **Hidden Redirect Window:** If a file download request unexpectedly returns `text/html` (a soft redirect or anti-hotlink gate), Meow spawns an invisible background `WebPage` instance (`createHiddenWindow`), lets the redirect resolve, and seamlessly captures the final binary payload.
* **Silent Mode with Shortcuts:** In *Silent Save* mode, Meow organizes downloaded files into subdirectories mirrored after the source URL structure while placing convenient `.lnk` / symlink shortcuts in the root download folder.
* **Shared Cookie Store:** Synchronizes cookies between Chromium's `QWebEngineCookieStore` and Qt's `QNetworkCookieJar`, allowing `QNetworkAccessManager` downloads to inherit full session authentication.

### 4. 🔨 BanHammer 2.0 — Built-in UserScript Engine & Ad Blocker
Meow features an integrated UserScript loader (`%APPDATA%/Meow/userscripts`) with pre-execution syntax validation via `QJSEngine` and automatic injection across all main frames and `<iframe>` subframes.
It ships with **BanHammer 2.0**, a four-stage defense script against modern web clutter:
1. **Instant Cosmetic CSS Shield:** Hides ad slots (Yandex RTB, AdFox, Google Ads, Taboola, Outbrain, MGID, Criteo, Buzzoola, Relap) before the first paint.
2. **Network Interceptor:** Hooks `window.fetch`, `XMLHttpRequest`, and `navigator.sendBeacon` to drop telemetry and RTB auction calls before they hit the network.
3. **Ad-Token (`erid`) Card Climber:** Detects mandatory ad-marking tokens (`erid=`, `ord.yandex.ru`, `ord.vk.com`, `ord.ozon.ru`) and collapses the entire parent ad card, defeating obfuscated CSS class names.
4. **Reactive `MutationObserver`:** Continuously cleans dynamically injected nodes in single-page applications (SPAs) with minimal CPU overhead.

### 5. 🛡️ Granular Privacy & Incognito Controls
* **Per-Tab Privacy Modes:** Mix normal and Incognito tabs freely within the same window.
* **Hardened Incognito Mode:** Uses an off-the-record profile, disables JavaScript execution by default, blocks third-party cookies, and automatically upgrades `http://` requests to `https://`.
* **Custom User-Agent & Proxy:** Configure system-wide, manual HTTP proxy, or custom `User-Agent` strings on the fly.
* **Single-Instance Guard:** Uses `QSharedMemory` (`meow_single`) and crash-recovery detection (`FatalExit` flag) to prevent duplicate processes and recover cleanly from unexpected terminations.

### 6. 🔍 Context-Aware Search & Translation Workflows
* **One-Click Reverse Image Search:** Right-click any image on a page to search for it directly in Google Lens / Yandex Images.
* **Instant Selection Translation:** Highlight text either on a webpage or right inside the URL bar and translate it in a new tab via your configured translation engine.
* **Frequency-Ranked SQLite Omnibox:** Autocompletes both page titles and URLs from a local SQLite database (`history.sqlite`), ranked by visit frequency.

---

## 🛠️ Tech Stack & Requirements

* **Language:** C++17
* **Framework:** Qt 6 (tested on **Qt 6.10.x**)
* **Qt Modules:** `Core`, `Gui`, `Widgets`, `Network`, `Svg`, `Sql`, `Qml`, `WebEngineCore`, `WebEngineWidgets`
* **Build System:** CMake 3.16+ (Ninja / MSBuild)
* **Compiler:** MSVC 2022 (Windows) or GCC / Clang (Linux)
* **Platform Libraries:** `gdiplus.lib` (Windows, for EXIF property writing)

---

## 🚀 Building from Source

### Windows (MSVC 2022 + CMake)
```bash
git clone <repository-url>
cd meow
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.10/6.10.3/msvc2022_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all
```

### Installing UserScripts (BanHammer)
1. Launch **Meow Browser**.
2. Open the menu via the top-right corner button and select **Show settings**.
3. Under the **Privacy policy** section, click **Open user scripts folder** (`...`).
4. Copy `banhammer.js` (or any custom `.js` script) into the opened `userscripts` directory.
5. Click **Reload user scripts** in Settings.

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl + T` | Open new standard tab |
| `Ctrl + Shift + N` | Open new Incognito tab |
| `Ctrl + J` | Open Downloads history & file manager |
| `Ctrl + S` | Archive full page with assets (`Save page`) |
| `Ctrl + Shift + I` | Open docked Web Inspector (DevTools) |
| `Ctrl + Enter` | Load URL / Execute search from address bar |
| `F5` | Reload page (bypasses cache if URL is unchanged) |

---

## 📁 Project Structure

* `mainwindow.*` — Tab management, corner menu action bar, animated toast notifications, and proxy setup.
* `webpage.*` — `QWebEngineView` wrapper, context menus, Omnibox completer, DevTools dock, and JS bridge.
* `filemanager.*` — Full-page DOM archiver, MIME-type resolution, soft-redirect detector, and silent-save organizer.
* `filemetadata.*` — GDI+ image encoder integration for writing `TagXPKeywords` and `TagExifUserComment` EXIF tags.
* `downloadmanager.*` — `QNetworkAccessManager` downloader with shared cookie store synchronization.
* `settingspage.*` — SQLite-backed (`settings.sqlite` & `history.sqlite`) configuration and history manager.
* `banhammer.js` — Multi-layer ad-blocking UserScript.

---

## 👤 Author & License

* **Author:** Mist Poryvaev
* **Version:** 0.1.0 (Qt6 Port)
