/***************************************************************************
 *   Copyright (C) 2025 by santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QSettings>
#include <QPushButton>
#include <QNetworkReply>

#include "installer.h"
#include "componentlist.h"
#include "installitem.h"
#include "mainwindow.h"
#include "qzipreader.h"
#include "utils.h"
#include "wasmsupport.h"

Installer::Installer( QWidget* parent )
         : QWidget( parent )
{
    setupUi(this);

    m_checkUpdates = true; //false;
    m_listLoaded = false;
    m_changed = false;

    m_installItem = nullptr;

    installTable->horizontalHeader()->setStretchLastSection( true );

    m_compsUrl = "https://simulide.com/p/direct_downloads/components/";
#ifdef __EMSCRIPTEN__
    // Browsers block downloads from other sites (CORS): the web build reads
    // a mirror of the component sets published next to the app.
    m_compsUrl = WasmSupport::pageBaseUrl()+"components/";

    QPushButton* zipButton = new QPushButton( tr("Install from .zip file..."), this );
    zipButton->setToolTip( tr("Install a component set downloaded from simulide.com") );
    if( QLayout* l = layout() ) l->addWidget( zipButton );
    QObject::connect( zipButton, &QPushButton::clicked, [this](){ installFromZipFile(); } );
#endif

    m_compsDir.setPath( MainWindow::self()->getConfigPath("components") );
    if( !m_compsDir.exists() ) m_compsDir.mkpath(".");

    loadList();

    QSettings* settings = MainWindow::self()->settings();
    QStringList installedList = settings->value("library/installed").toString().split(",");

    for( QString itemStr : installedList )
    {
        if( itemStr.isEmpty() ) continue;

        QStringList list = itemStr.split(";");
        if( list.size() < 2 ) return;
        QString name = list.first();

        InstallItem* item = m_items.value( name );
        if( !item ) continue;

        QString version = "2507102250" ;//list.at(1);//
        int64_t v = version.toLongLong();
        item->m_version = v;
        item->shouldUpdate( v );

        qDebug() << "Installer::Installer" << name << version;

        m_installed.insert( item->m_name, v );
    }
}

void Installer::loadList()
{
    QString compFile = m_compsDir.filePath("components.txt");
    QString replyStr = fileToString( compFile, "Installer::loadList" );

    QStringList setList = replyStr.split("\n"); // List of Component Sets

    m_changed = false;
    m_version = 0;
    int row = 0;
    for( QString itemStr : setList )
    {
        if( itemStr.isEmpty() ) continue;
        //qDebug() <<"Installer::loadList"<< itemStr;

        InstallItem* item = nullptr;

        QStringList list = itemStr.split(";");
        QString name = list.first();

        if( m_installed.contains( name ) )
        {
            item = m_items.value( name );

            if( item && list.size() > 3 ) {
                QString versionStr = list.at(3);
                versionStr.remove(0,1);
                //qDebug() << "Installer::loadList updated" << name << v << item->m_version;

                uint64_t version = versionStr.toLongLong();
                if( item->shouldUpdate( version ) ) m_changed = true;
                if( version > m_version ) m_version = version;
            }
        }
        else if( m_items.contains( name ) )
        {
            item = m_items.value( name );
        }
        else
        {
            item = new InstallItem( this, itemStr );

            item->shouldUpdate( 0 );

            m_items.insert( item->m_name, item );

            installTable->insertRow( row );
            installTable->setCellWidget( row, 0, item );

            m_changed = true;
        }
        float scale = MainWindow::self()->fontScale();

        if( item->m_file.isEmpty() ) installTable->setRowHeight( row, 25*scale );
        else                         installTable->setRowHeight( row, 60*scale );

        row++;
    }
    installTable->setRowCount( row );
    m_listLoaded = true;
}

void Installer::loadInstalled()
{
    for( QString item : m_installed.keys() )
    {
        QDir compSetDir = m_compsDir;
        compSetDir.cd( item );
        ComponentList::self()->LoadCompSetAt( compSetDir );
        QStringList itemList = ComponentList::self()->getxmlItems();
        m_groupItemList.insert( item, itemList );
    }
}

void Installer::checkForUpdates( QString url )
{
    if( !m_checkUpdates ) return;

    //QString version = MainWindow::self()->settings()->value("library/version").toString();
    //qDebug() << "version" << version;
    qDebug() << "Checking for Updates...";

#ifdef __EMSCRIPTEN__
    if( url.isEmpty() ) url = m_compsUrl+"components.txt";
#else
    if( url.isEmpty() ) url = m_compsUrl+"dloadset.php?file=components.txt";
#endif
    QNetworkRequest request( url );

    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute
                        , QNetworkRequest::NoLessSafeRedirectPolicy );

    request.setTransferTimeout( 5000 );

    m_reply = m_manager.get( request );

    QObject::connect( m_reply, &QNetworkReply::finished,
                     [=](){ updtReady(); } );
}

void Installer::updtReady()
{
    if( m_reply->error() == QNetworkReply::NoError )
    {
        QString compFile = m_compsDir.filePath( "components.txt" );
        QFile file( compFile );
        if( file.exists() ) QFile::remove( compFile );

        if( file.open( QIODevice::WriteOnly | QIODevice::Append ))
        {
            file.write( m_reply->readAll() );
            file.close();

            loadList();

            if( m_changed ) qDebug() << "Installer: Updates available";
            else            qDebug() << "Installer: Up to date";
        }else{
            qDebug() << "Installer::updtReady ERROR: can't write file" << compFile ;
            m_installItem = nullptr;
        }
    }
    else qDebug() << "Installer::updtReady ERROR:" << m_reply->errorString(); // There was a network error

    m_reply->close();
}

void Installer::installItem( QString itemName )
{
    if( m_installItem ) // A download is running: install this one after it
    {
        if( !m_queue.contains( itemName ) && m_installItem->m_name != itemName ) m_queue.append( itemName );
        return;
    }
    InstallItem* item = m_items.value( itemName );
    if( !item ) return;

    if( !item->m_depends.isEmpty() && !m_installed.contains( item->m_depends ) )
    {
        m_queue.prepend( itemName );      // install the dependency first
        itemName = item->m_depends;
        item = m_items.value( itemName );
        if( !item ) { m_queue.removeFirst(); return; }
    }

    qDebug() << "Installing Component Set:" << itemName;

#ifdef __EMSCRIPTEN__
    QString url = m_compsUrl+item->m_file;
#else
    QString url = m_compsUrl+"dloadset.php?file="+item->m_file;
#endif
    QNetworkRequest request( url );

    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute
                        , QNetworkRequest::NoLessSafeRedirectPolicy );

    request.setTransferTimeout( 30000 );

    m_installItem = item;
    QNetworkReply* reply = m_manager.get( request );
    QObject::connect( reply, &QNetworkReply::finished, [this, reply, item](){ itemDataReady( reply, item ); } );
}

void Installer::unInstallItem( QString itemName )
{
    QDir compSetDir = m_compsDir;
    compSetDir.cd( itemName );
    compSetDir.removeRecursively();
    m_installed.remove( itemName );

    ComponentList::self()->createList();
}

void Installer::itemDataReady( QNetworkReply* reply, InstallItem* item )
{
    QByteArray data;
    bool ok = reply->error() == QNetworkReply::NoError;
    if( ok ) data = reply->readAll();  // Read before close(): closed replies return no data
    else     qDebug() << "Installer::itemDataReady network ERROR:" << reply->errorString();
    reply->close();
    reply->deleteLater();

    QString itemName = item->m_name;
    if( ok && data.size() )
    {
        QString zipFile = m_compsDir.filePath( item->m_file );
        QFile file( zipFile );
        if( file.exists() ) QFile::remove( zipFile );

        if( file.open( QIODevice::WriteOnly ) )
        {
            file.write( data );
            file.close();

            QDir compSetDir = m_compsDir;
            if( compSetDir.cd( itemName ) ) compSetDir.removeRecursively();

            qZipReader qZip( zipFile );
            if( qZip.extractAll( m_compsDir.absolutePath() ) )
            {
                m_installed.insert( itemName, item->m_versionNext );
                item->updated();
                qDebug() << itemName <<"Installed";
#ifdef __EMSCRIPTEN__
                writeSettings(); // No "close app" event in a browser. (QSettings saves itself shortly after)
#endif
                ComponentList::self()->createList();
            }else{
                qDebug() << itemName << "Installer::itemDataReady ERROR extracting" << zipFile ;
                m_installed.remove( itemName );
            }
            QFile::remove( zipFile );
        }
        else qDebug() << "Installer::itemDataReady ERROR: can't write file" << zipFile ;
    }
    if( !m_installed.contains( itemName ) ) { item->m_version = 0; item->shouldUpdate( 0 ); } // Show "Install" again

    m_installItem = nullptr;
    // Next in queue (drop items whose dependency just failed)
    while( !m_queue.isEmpty() )
    {
        QString next = m_queue.takeFirst();
        InstallItem* nextItem = m_items.value( next );
        if( !nextItem ) continue;
        if( !nextItem->m_depends.isEmpty() && nextItem->m_depends == itemName && !m_installed.contains( itemName ) )
        { nextItem->m_version = 0; nextItem->shouldUpdate( 0 ); continue; }
        installItem( next );
        break;
    }
}

void Installer::on_updtButton_clicked()
{
    checkForUpdates();
}

void Installer::writeSettings()
{
    QString installed; // = "Arduino,AVR,PIC,MCS51,MCS65,Z80,Analog,74,Digipot,Tools";

    for( QString itemName : m_installed.keys() )
    {
        InstallItem* item = m_items.value( itemName );
        installed += item->toString()+",";
    }
    QSettings* settings = MainWindow::self()->settings();
    settings->setValue("library/installed", installed );
}


#ifdef __EMSCRIPTEN__
void Installer::installFromZipFile()
{
    // Component set zip files contain one top folder with .comp files.
    // Extract into the user components folder, which is always scanned.
    QString tmpDir = MainWindow::self()->getConfigPath("tmp");

    WasmSupport::pickFile( "*", tmpDir, [this]( const QString& zipFile ){ installZip( zipFile ); } );
}

void Installer::installZip( const QString& zipFile )
{
    QString userComps = MainWindow::self()->userPath()+"/components";
    QDir().mkpath( userComps );
    qZipReader qZip( zipFile );
    bool ok = qZip.extractAll( userComps );
    QFile::remove( zipFile );

    if( ok ) qDebug() << "Installed component set from" << zipFile.split("/").last();
    else     qDebug() << "ERROR: could not extract" << zipFile.split("/").last();

    ComponentList::self()->createList();
    WasmSupport::syncFs();
}
#endif
