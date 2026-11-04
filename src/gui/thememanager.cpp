/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

//#include <QDebug>

#include <QGuiApplication>
#include <QStyleHints>
#include <QTimer>

#include "thememanager.h"
#include "mainwindow.h"
#include "componentlist.h"
#include "circuitwidget.h"
#include "editorwindow.h"
#include "circuit.h"

ThemeManager* ThemeManager::m_pSelf = nullptr;

ThemeManager::ThemeManager()
{
    m_pSelf = this;

    m_darkColors.resize( 4 );
    m_lightColors.resize( 4 );
    m_colors.resize( 4 );

    m_darkColors[0] = 0xFF1E1E28;  // Circuit back, Comp back
    m_darkColors[1] = 0xFF30303A;  // Circuit fore
    m_darkColors[2] = 0xFF000000;  // Comp back
    m_darkColors[3] = 0xFFD0D0C5;  // Comp fore, Pin, Conn

    m_lightColors[0] = 0xFFF0F0D2;
    m_lightColors[1] = 0xFFD2D2D2;
    m_lightColors[2] = 0xFFFFFFFF;
    m_lightColors[3] = 0xFF000000;

    for( uint i=0; i<m_colors.size(); ++i )
        m_colors[i] = m_lightColors[i];


    QPalette palette( QColor( 0xff2d2d2d ) );
    //qDebug() << palette.window().color().name();

    //palette.setColor( QPalette::Light          , QColor( 0xff404040 ) );
    //palette.setColor( QPalette::Midlight       , QColor( 0xff3c3c3c ) );
    //palette.setColor( QPalette::Mid            , QColor( 0xff1b1b1b ) );
    //palette.setColor( QPalette::Dark           , QColor( 0xff121212 ) );

    palette.setColor( QPalette::Active  , QPalette::WindowText  , QColor( 0xfff0f0f0 ) );
    palette.setColor( QPalette::Inactive, QPalette::WindowText  , QColor( 0xffd0d0d0 ) );
    //palette.setColor( QPalette::Disabled, QPalette::WindowText  , QColor( 0xff505050 ) );
    palette.setColor( QPalette::Active  , QPalette::Text        , QColor( 0xfff0f0e0 ) );
    palette.setColor( QPalette::Inactive, QPalette::Text        , QColor( 0xffd0d0c0 ) );
    palette.setColor( QPalette::Disabled, QPalette::Text        , QColor( 0xff606060 ) );
    //palette.setColor( QPalette::Active, QPalette::BrightText  , QColor( 0xffff0000 ) );
    palette.setColor( QPalette::Active  , QPalette::ButtonText  , QColor( 0xfff0f0e0 ) );
    palette.setColor( QPalette::Inactive, QPalette::ButtonText  , QColor( 0xffe0e0d0 ) );
    palette.setColor( QPalette::Disabled, QPalette::ButtonText  , QColor( 0xff808080 ) );


    palette.setColor( QPalette::Inactive, QPalette::Base        , QColor( 0xff25252E ) );
    palette.setColor( QPalette::Active  , QPalette::Base        , QColor( 0xff1E1E28 ) );
    palette.setColor( QPalette::Active  , QPalette::Window      , QColor( 0xff282828 ) );
    palette.setColor( QPalette::Active  , QPalette::Button      , QColor( 0xff323232 ) );

    //palette.setColor( QPalette::Shadow         , QColor( 0xff000000 ) );
    //palette.setColor( QPalette::AlternateBase  , QColor( 0xff141414 ) );
    //palette.setColor( QPalette::ToolTipBase    , QColor( 0xffffffdc ) );
    //palette.setColor( QPalette::ToolTipText    , QColor( 0xff000000 ) );
    //palette.setColor( QPalette::PlaceholderText, QColor( 0xffffffff ) );

    //palette.setColor( QPalette::Active, QPalette::Highlight, QColor( 0xff2828ff ) );
    //palette.setColor( QPalette::Disabled, QPalette::Window, QColor( 0xff282828 ) ); // Disabled icons

    m_darkPalette = palette;

    // An explicit light palette: a default QPalette follows the OS scheme,
    // which gave dark widgets in "Light" mode on devices set to dark.
    // Built by hand: Fusion's standardPalette() also turns dark when the OS is dark.
    QPalette light( Qt::black, QColor( 0xffefefef ), Qt::white, QColor( 0xffa0a0a0 ), QColor( 0xffb4b4b4 )
                  , Qt::black, Qt::white, Qt::white, QColor( 0xffefefef ) );
    light.setColor( QPalette::Button,          QColor( 0xffefefef ) );
    light.setColor( QPalette::ButtonText,      Qt::black );
    light.setColor( QPalette::AlternateBase,   QColor( 0xfff7f7f7 ) );
    light.setColor( QPalette::ToolTipBase,     QColor( 0xffffffdc ) );
    light.setColor( QPalette::ToolTipText,     Qt::black );
    light.setColor( QPalette::Highlight,       QColor( 0xff308cc6 ) );
    light.setColor( QPalette::HighlightedText, Qt::white );
    light.setColor( QPalette::PlaceholderText, QColor( 0xff808080 ) );
    light.setColor( QPalette::Link,            QColor( 0xff0000ff ) );
    light.setColor( QPalette::Disabled, QPalette::WindowText, QColor( 0xffa0a0a0 ) );
    light.setColor( QPalette::Disabled, QPalette::Text,       QColor( 0xffa0a0a0 ) );
    light.setColor( QPalette::Disabled, QPalette::ButtonText, QColor( 0xffa0a0a0 ) );
    m_lightPalette = light;

    QObject::connect( QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged
                    , [this](){ // Let Qt finish its own scheme change first, then set our palette
        if( m_currentTheme == Theme::System )
            QTimer::singleShot( 50, [this](){ applySystemScheme(); } );
    });
}
ThemeManager::~ThemeManager(){}

void ThemeManager::setTheme( Theme theme )
{
    if( m_applied && m_currentTheme == theme ) return;
    m_currentTheme = theme;

    if( theme == Theme::System ) applySystemScheme();
    else                         applyDark( theme == Theme::Dark );
}

void ThemeManager::applySystemScheme()
{
    // Ask the OS, not the app's own forced scheme
    QGuiApplication::styleHints()->unsetColorScheme();
    applyDark( QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark );
}

void ThemeManager::applyDark( bool dark )
{
    m_applied = true;
    m_isDark = dark;

    if( m_currentTheme != Theme::System ) // Keep Qt's own widgets on the chosen scheme
        QGuiApplication::styleHints()->setColorScheme( dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light );

    QApplication::setPalette( m_isDark ? m_darkPalette : m_lightPalette );

    for( uint i=0; i<m_colors.size(); ++i )
        m_colors[i] = m_isDark? m_darkColors[i] : m_lightColors[i];

    ComponentList::self()->setTheme( m_isDark );
    CircuitWidget::self()->updateIcons();
    EditorWindow::self()->updateIcons();
    MainWindow::self()->updateIcons();

    Circuit::self()->update();
}

uint32_t* ThemeManager::getColorPtr( uint index )
{
    if( index >= m_colors.size() ) index = 0;
    return &m_colors[index];
}

QIcon ThemeManager::icon( QString iconFile )
{
    if( m_isDark ) return invertPixmap( QPixmap(iconFile) );
    else           return QIcon( iconFile );
}

QPixmap ThemeManager::invertPixmap( QPixmap pixmap )
{
    QImage image = pixmap.toImage();
    image = image.convertToFormat( QImage::Format_ARGB32 );

    bool ok = true;
    for( int y=0; y<image.height(); ++y ) {
        for( int x=0; x<image.width(); ++x ) {
            QRgb pixel = image.pixel( x, y );
            if( qAlpha( pixel ) == 0) continue;
            if( !qIsGray( pixel ) ) { ok = false; break; }

            int gray = 255-qBlue( pixel );
            if( gray > 235 ) gray = 235;
            image.setPixel( x, y, qRgba(gray, gray, gray*.9, qAlpha(pixel)) );
        }
    }
    if( ok ) return QPixmap::fromImage( image ) ;
    else     return pixmap;
}
