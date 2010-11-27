#include "playlistmanager.h"

#include <QVBoxLayout>

#include "audioengine.h"
#include "collectionmodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "queueview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"

#include "infowidgets/sourceinfowidget.h"

#define FILTER_TIMEOUT 280


PlaylistManager::PlaylistManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QWidget() )
    , m_currentProxyModel( 0 )
    , m_currentModel( 0 )
    , m_currentView( 0 )
    , m_currentMode( 0 )
    , m_superCollectionVisible( true )
{
    m_stack = new QStackedWidget();

    m_queueView = new QueueView();
    m_queueModel = new PlaylistModel( m_queueView );
    m_queueView->queue()->setModel( m_queueModel );
    APP->audioEngine()->setQueue( m_queueView->queue()->proxyModel() );

    m_widget->setLayout( new QVBoxLayout() );

    m_splitter = new QSplitter();
    m_splitter->setOrientation( Qt::Vertical );
    m_splitter->setChildrenCollapsible( false );

    m_splitter->addWidget( m_stack );
    m_splitter->addWidget( m_queueView );
    m_splitter->setStretchFactor( 0, 3 );
    m_splitter->setStretchFactor( 1, 1 );

    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->addWidget( m_splitter );

    m_superCollectionView = new CollectionView();
    m_superCollectionFlatModel = new CollectionFlatModel( m_superCollectionView );
    m_superCollectionView->setModel( m_superCollectionFlatModel );

    m_stack->addWidget( m_superCollectionView );
    m_currentView = m_superCollectionView;

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
}


PlaylistManager::~PlaylistManager()
{
    delete m_widget;
}


PlaylistView*
PlaylistManager::queue() const
{
    return m_queueView->queue();
}


bool
PlaylistManager::show( const Tomahawk::playlist_ptr& playlist )
{
    unlinkPlaylist();

    if ( !m_playlistViews.contains( playlist ) )
    {
        PlaylistView* view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );

        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();

        model->loadPlaylist( playlist );
        playlist->resolve();

        m_playlistViews.insert( playlist, view );
        m_views.insert( (PlaylistInterface*)m_currentProxyModel, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
        m_currentView = view;
    }
    else
    {
        PlaylistView* view = m_playlistViews.value( playlist );
        m_stack->setCurrentWidget( view );
        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();
        m_currentView = view;
    }

    m_superCollectionVisible = false;
    linkPlaylist();

    emit numSourcesChanged( APP->sourcelist().count() );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::album_ptr& album )
{
    qDebug() << Q_FUNC_INFO << &album << album.data();
    unlinkPlaylist();

    if ( !m_albumViews.contains( album ) )
    {
        PlaylistView* view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );

        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();

        model->loadAlbum( album );

        m_albumViews.insert( album, view );
        m_views.insert( (PlaylistInterface*)m_currentProxyModel, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
        m_currentView = view;
    }
    else
    {
        PlaylistView* view = m_albumViews.value( album );
        m_stack->setCurrentWidget( view );
        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();
        m_currentView = view;
    }

    m_superCollectionVisible = false;
    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::collection_ptr& collection )
{
    unlinkPlaylist();

    if ( !m_collectionViews.contains( collection ) )
    {
        CollectionView* view = new CollectionView();
        CollectionFlatModel* model = new CollectionFlatModel();
        view->setModel( model );

        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();

        model->addCollection( collection );

        m_collectionViews.insert( collection, view );
        m_views.insert( (PlaylistInterface*)m_currentProxyModel, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
        m_currentView = view;
    }
    else
    {
        CollectionView* view = m_collectionViews.value( collection );
        m_stack->setCurrentWidget( view );
        m_currentProxyModel = view->proxyModel();
        m_currentModel = view->model();
        m_currentView = view;
    }

    m_superCollectionVisible = false;
    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::source_ptr& source )
{
    unlinkPlaylist();

    m_currentProxyModel = 0;
    m_currentModel = 0;
    m_currentView = 0;

    if ( !m_sourceViews.contains( source ) )
    {
        SourceInfoWidget* swidget = new SourceInfoWidget( source );
        m_currentInfoWidget = swidget;
        m_stack->addWidget( m_currentInfoWidget );
        m_sourceViews.insert( source, swidget );
    }
    else
    {
        m_currentInfoWidget = m_sourceViews.value( source );
    }

    m_stack->setCurrentWidget( m_currentInfoWidget );
    m_superCollectionVisible = false;

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::showSuperCollection()
{
    foreach( const Tomahawk::source_ptr& source, APP->sourcelist().sources() )
    {
        if ( !m_superCollections.contains( source->collection() ) )
        {
            m_superCollections.append( source->collection() );
            m_superCollectionFlatModel->addCollection( source->collection() );
        }
    }

    m_stack->setCurrentWidget( m_superCollectionView );
    m_currentProxyModel = m_superCollectionView->proxyModel();
    m_currentModel = m_superCollectionView->model();
    m_currentView = m_superCollectionView;
    m_views.insert( (PlaylistInterface*)m_currentProxyModel, m_superCollectionView );

    m_superCollectionVisible = true;
    linkPlaylist();

    emit numSourcesChanged( m_superCollections.count() );
    return true;
}


void
PlaylistManager::setTreeMode()
{
    return;

    qDebug() << Q_FUNC_INFO;

    m_currentMode = 1;
    m_stack->setCurrentWidget( m_superCollectionView );
}


void
PlaylistManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 0;
    m_stack->setCurrentWidget( m_superCollectionView );
}


void
PlaylistManager::showQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "showQueue",
                                   Qt::QueuedConnection );
        return;
    }

    m_queueView->showQueue();
}


void
PlaylistManager::hideQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "hideQueue",
                                   Qt::QueuedConnection );
        return;
    }

    m_queueView->hideQueue();
}


void
PlaylistManager::setFilter( const QString& filter )
{
    m_filter = filter;

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
PlaylistManager::applyFilter()
{
    qDebug() << Q_FUNC_INFO;

    if ( m_currentProxyModel )
        m_currentProxyModel->setFilterRegExp( m_filter );
}


void
PlaylistManager::unlinkPlaylist()
{
    if ( m_currentModel )
    {
        disconnect( m_currentModel,      SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                  SLOT( onTrackCountChanged( unsigned int ) ) );
    }

    if ( m_currentProxyModel )
    {
        disconnect( m_currentProxyModel, SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

        disconnect( m_currentProxyModel, SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                    this,                SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        disconnect( m_currentProxyModel, SIGNAL( shuffleModeChanged( bool ) ),
                    this,                SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
PlaylistManager::linkPlaylist()
{
    connect( m_currentModel,      SIGNAL( trackCountChanged( unsigned int ) ),
             this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

    connect( m_currentProxyModel, SIGNAL( trackCountChanged( unsigned int ) ),
             this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

    connect( m_currentProxyModel, SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
             this,                SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

    connect( m_currentProxyModel, SIGNAL( shuffleModeChanged( bool ) ),
             this,                SIGNAL( shuffleModeChanged( bool ) ) );

    applyFilter();
    APP->audioEngine()->setPlaylist( (PlaylistInterface*)m_currentProxyModel );

    emit numTracksChanged( m_currentModel->trackCount() );
    emit repeatModeChanged( m_currentProxyModel->repeatMode() );
    emit shuffleModeChanged( m_currentProxyModel->shuffled() );
}


void
PlaylistManager::onTrackCountChanged( unsigned int )
{
    emit numTracksChanged( m_currentModel->trackCount() );
    emit numShownChanged( m_currentProxyModel->trackCount() );
}


void
PlaylistManager::setRepeatMode( PlaylistInterface::RepeatMode mode )
{
    if ( m_currentProxyModel )
        m_currentProxyModel->setRepeatMode( mode );
}


void
PlaylistManager::setShuffled( bool enabled )
{
    if ( m_currentProxyModel )
        m_currentProxyModel->setShuffled( enabled );
}


void
PlaylistManager::showCurrentTrack()
{
    PlaylistInterface* playlist = APP->audioEngine()->currentTrackPlaylist();

    if ( m_views.contains( playlist ) )
    {
        unlinkPlaylist();

        m_currentView = m_views.value( playlist );
        m_currentProxyModel = m_currentView->proxyModel();
        m_currentModel = m_currentView->model();

        m_stack->setCurrentWidget( m_currentView );
        linkPlaylist();
    }

    if ( m_currentView && m_currentProxyModel )
        m_currentView->scrollTo( m_currentProxyModel->currentItem(), QAbstractItemView::PositionAtCenter );
}
