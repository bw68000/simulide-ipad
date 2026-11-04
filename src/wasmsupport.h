/***************************************************************************
 *   Browser (WebAssembly) support for SimulIDE                            *
 *   ( see copyright.txt file at root folder )                             *
 ***************************************************************************/

#pragma once

#ifdef __EMSCRIPTEN__

#include <QString>
#include <functional>

class QApplication;
class QWidget;

namespace WasmSupport
{
    // Long-press => context menu, periodic save of the browser file system.
    void init( QApplication* app );

    // Write pending file changes to browser storage (IndexedDB).
    void syncFs();

    // Let the user pick a file from the device; it is copied into destDir
    // and onReady() receives the local path.
    void pickFile( const QString& filter, const QString& destDir,
                   std::function<void(const QString&)> onReady );

    // Offer a local file to the user as a download ("Save to Files" on iPad).
    void downloadFile( const QString& path );

    // URL of the folder the web page was loaded from (ends with "/").
    QString pageBaseUrl();

    // Component sets bundled with the web app (from SimulIDE 1.1.0):
    // downloaded once into destDir, again only when the build changes.
    void ensureBuiltinData( const QString& destDir, std::function<void()> onInstalled );
}

#endif
