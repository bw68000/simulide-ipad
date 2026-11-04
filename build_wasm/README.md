# SimulIDE for iPad (web version)

This folder builds SimulIDE as a web app with Qt for WebAssembly. It runs in
Safari on iPad (and in any modern browser). Add it to the Home Screen and it
opens full screen like a normal app.

## Using it on iPad

1. Open the site in Safari.
2. Tap **Share → Add to Home Screen**. Launch SimulIDE from that icon from
   now on. Your settings, installed components and circuits are kept in the
   browser's storage, and Safari keeps that storage much longer for Home
   Screen apps.
3. **Touch controls**
   - Drag components from the list onto the circuit.
   - Drag from a pin to another pin to draw a wire.
   - **Touch and hold** a component (or an empty spot) for the right-click
     menu: properties, rotate, delete…
   - Use the zoom buttons in the toolbar to zoom.
4. **Microcontrollers (Arduino, AVR, PIC…)** come as component sets: open
   the second tab in the left panel and tap the install button of a set.
   If the site has no mirrored sets, download a set's .zip on simulide.com
   and tap **Install from .zip file…**.
5. **Files**
   - *Open*: picks a .sim1/.sim2 file from the Files app.
   - *Save As*: stores the circuit in the app and also offers it as a
     download, so you can keep a copy in Files / iCloud.
   - *Save*: stores the circuit in the app.

## What doesn't work in the browser

- **Compiling code** (Arduino IDE, avr-gcc, SDCC, gputils…). Browsers can't
  run other programs. Compile on a computer, then load the `.hex` into the
  microcontroller with *Load firmware*.
- **Serial port** connection to real hardware.
- **ESP32 / STM32** (they need the external QEMU program).
- The simulation runs on one thread, so very heavy circuits run slower than
  on a desktop.

## Building

GitHub Actions builds and publishes it on GitHub Pages
(`.github/workflows/web.yml`). Turn on *Settings → Pages → Source: GitHub
Actions* in the repository.

To build locally on Linux:

```sh
pip install aqtinstall
aqt install-qt linux desktop 6.8.3 linux_gcc_64 -O ~/qt
aqt install-qt all_os wasm 6.8.3 wasm_singlethread -m qtmultimedia -O ~/qt
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install 3.1.56 && ~/emsdk/emsdk activate 3.1.56
source ~/emsdk/emsdk_env.sh
build_wasm/build.sh ~/qt/6.8.3
python3 build_wasm/tools/mirror_components.py build_wasm/out/dist   # optional
cd build_wasm/out/dist && python3 -m http.server 8000
```

## Changes compared to the desktop source

All browser-only changes are behind `#ifdef __EMSCRIPTEN__` or `wasm { }` in
`SimulIDE.pri`. The desktop build is unaffected.

- `src/angel/include/as_sim_wrap.h`: AngelScript can't call C++ functions
  natively in WebAssembly, so script bindings use generic wrappers there.
- `src/simulator/simulator.*`: the simulation step runs on the main thread
  (no threads).
- `src/wasm_stubs/QProcess`: external programs always "fail to start".
- `src/wasmsupport.*`: touch-and-hold context menu, browser storage, file
  open/download.
- `src/gui/componentlist/installer.*`: component sets come from the site's
  own mirror, or a local .zip file.
- `src/components/outputs/audio_out.*` and `setCodec` calls: Qt 6 versions.
- Serial port component left out.
