/***************************************************************************
 *   Browser (WebAssembly) support for SimulIDE                            *
 *   ( see copyright.txt file at root folder )                             *
 ***************************************************************************/

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "wasmsupport.h"
#include "qzipreader.h"

namespace
{
// Touch screens have no right button: holding a finger still opens the
// context menu, like on any other iPad app.
class LongPressFilter : public QObject
{
    public:
        LongPressFilter( QObject* parent ) : QObject( parent )
        {
            m_timer.setSingleShot( true );
            m_timer.setInterval( 550 );
            QObject::connect( &m_timer, &QTimer::timeout, this, [this](){ fire(); } );
        }

    protected:
        bool eventFilter( QObject* obj, QEvent* event ) override
        {
            switch( event->type() )
            {
            case QEvent::MouseButtonPress:
            {
                QMouseEvent* me = static_cast<QMouseEvent*>( event );
                if( me->source() == Qt::MouseEventNotSynthesized ) break; // real mouse: has right button
                if( me->button() != Qt::LeftButton ) break;
                QWidget* w = qobject_cast<QWidget*>( obj );
                if( !w ) break;
                m_target = w;
                m_localPos  = me->position().toPoint();
                m_globalPos = me->globalPosition().toPoint();
                m_timer.start();
                break;
            }
            case QEvent::MouseMove:
            {
                if( !m_timer.isActive() ) break;
                QMouseEvent* me = static_cast<QMouseEvent*>( event );
                if( (me->globalPosition().toPoint()-m_globalPos).manhattanLength() > 12 ) m_timer.stop();
                break;
            }
            case QEvent::MouseButtonRelease:
                m_timer.stop();
                if( m_swallowRelease ) { m_swallowRelease = false; return true; }
                break;
            default: break;
            }
            return QObject::eventFilter( obj, event );
        }

    private:
        void fire()
        {
            QWidget* w = m_target.data();
            if( !w ) return;
            // End the press first so nothing stays "grabbed" (dragging an item)
            QMouseEvent release( QEvent::MouseButtonRelease, QPointF( m_localPos ), QPointF( m_globalPos ),
                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier );
            QApplication::sendEvent( w, &release );
            m_swallowRelease = true;

            QContextMenuEvent cme( QContextMenuEvent::Mouse, m_localPos, m_globalPos );
            QApplication::sendEvent( w, &cme );
        }

        QTimer m_timer;
        QPointer<QWidget> m_target;
        QPoint m_localPos;
        QPoint m_globalPos;
        bool m_swallowRelease = false;
};
}

void WasmSupport::init( QApplication* app )
{
    app->installEventFilter( new LongPressFilter( app ) );

    // Persist settings, installed components and saved circuits.
    QTimer* syncTimer = new QTimer( app );
    QObject::connect( syncTimer, &QTimer::timeout, [](){ syncFs(); } );
    syncTimer->start( 4000 );
}

void WasmSupport::syncFs()
{
    EM_ASM({
        if( typeof IDBFS === 'undefined' || !Module.simulidePersist ) return;
        if( Module.simulideSyncing ) return;
        Module.simulideSyncing = true;
        FS.syncfs( false, function( err ){
            Module.simulideSyncing = false;
            if( err ) console.log( "SimulIDE: storage sync error", err );
        });
    });
}

void WasmSupport::pickFile( const QString& filter, const QString& destDir,
                            std::function<void(const QString&)> onReady )
{
    QFileDialog::getOpenFileContent( filter,
        [destDir, onReady]( const QString& fileName, const QByteArray& content )
        {
            if( fileName.isEmpty() ) return;          // cancelled
            QDir().mkpath( destDir );
            QString path = QDir( destDir ).filePath( QFileInfo( fileName ).fileName() );
            QFile file( path );
            if( !file.open( QIODevice::WriteOnly ) ) return;
            file.write( content );
            file.close();
            onReady( path );
            syncFs();
        });
}

void WasmSupport::downloadFile( const QString& path )
{
    QFile file( path );
    if( !file.open( QIODevice::ReadOnly ) ) return;
    QFileDialog::saveFileContent( file.readAll(), QFileInfo( path ).fileName() );
}

void WasmSupport::ensureBuiltinData( const QString& destDir, std::function<void()> onInstalled )
{
    const QString version = QStringLiteral( APP_VERSION "-" REVNO );
    QFile mark( destDir+"/.builtin_version" );
    if( mark.open( QIODevice::ReadOnly ) && QString::fromUtf8( mark.readAll() ) == version ) return;
    mark.close();

    static QNetworkAccessManager* manager = new QNetworkAccessManager( qApp );
    QNetworkRequest request( QUrl( pageBaseUrl()+"builtin/data.zip" ) );
    request.setTransferTimeout( 60000 );
    QNetworkReply* reply = manager->get( request );

    QObject::connect( reply, &QNetworkReply::finished, [reply, destDir, version, onInstalled]()
    {
        QByteArray data;
        if( reply->error() == QNetworkReply::NoError ) data = reply->readAll();
        else qDebug() << "Built-in components: download error" << reply->errorString();
        reply->close();
        reply->deleteLater();
        if( data.isEmpty() ) return;

        QDir dir( destDir );
        dir.removeRecursively();             // Drop the previous version
        QDir().mkpath( destDir );
        QString zipPath = destDir+"/../builtin_data.zip";
        QFile zip( zipPath );
        if( !zip.open( QIODevice::WriteOnly ) ) return;
        zip.write( data );
        zip.close();

        qZipReader reader( zipPath );
        bool ok = reader.extractAll( destDir );
        QFile::remove( zipPath );
        if( !ok ) { qDebug() << "Built-in components: could not extract"; return; }

        QFile m( destDir+"/.builtin_version" );
        if( m.open( QIODevice::WriteOnly ) ) { m.write( version.toUtf8() ); m.close(); }
        onInstalled();
        syncFs();
    });
}

QString WasmSupport::pageBaseUrl()
{
    const char* url = emscripten_run_script_string(
        "(function(){ var h = window.location.href.split('#')[0].split('?')[0];"
        " return h.substring( 0, h.lastIndexOf('/')+1 ); })()" );
    return QString::fromUtf8( url );
}

#endif
