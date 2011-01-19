#include "trackproxymodel.h"

#include <QDebug>
#include <QTreeView>

#include "album.h"
#include "query.h"
#include "collectionmodel.h"


TrackProxyModel::TrackProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , PlaylistInterface( this )
    , m_model( 0 )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceModel( 0 );
}


void
TrackProxyModel::setSourceModel( TrackModel* sourceModel )
{
    m_model = sourceModel;

    if ( m_model )
        connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ),
                          SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

    QSortFilterProxyModel::setSourceModel( sourceModel );
}


void
TrackProxyModel::setFilter( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;
    setFilterRegExp( pattern );

    emit filterChanged( pattern );
    emit trackCountChanged( trackCount() );
}


Tomahawk::result_ptr
TrackProxyModel::siblingItem( int itemsAway )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = index( 0, 0 );
    if( rowCount() )
    {
        if ( m_shuffled )
        {
            // random mode is enabled
            // TODO come up with a clever random logic, that keeps track of previously played items
            idx = index( qrand() % rowCount(), 0 );
        }
        else if ( currentItem().isValid() )
        {
            idx = currentItem();

            // random mode is disabled
            if ( m_repeatMode == PlaylistInterface::RepeatOne )
            {
                // repeat one track
                idx = index( idx.row(), 0 );
            }
            else
            {
                // keep progressing through the playlist normally
                idx = index( idx.row() + itemsAway, 0 );
            }
        }
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistInterface::RepeatAll )
    {
        // repeat all tracks
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = index( 0, 0 );
        }
        else
        {
            // reset to last item
            idx = index( rowCount() - 1, 0 );
        }
    }

    // Try to find the next available PlaylistItem (with results)
    if ( idx.isValid() ) do
    {
        PlItem* item = itemFromIndex( mapToSource( idx ) );
        qDebug() << item->query()->toString();
        if ( item && item->query()->numResults() )
        {
            qDebug() << "Next PlaylistItem found:" << item->query()->toString() << item->query()->results().at( 0 )->url();
            setCurrentItem( idx );
            return item->query()->results().at( 0 );
        }

        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }
    while ( idx.isValid() );

    setCurrentItem( QModelIndex() );
    return Tomahawk::result_ptr();
}


bool
TrackProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( filterRegExp().isEmpty() )
        return true;

    PlItem* pi = itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    const Tomahawk::query_ptr& q = pi->query();
    Tomahawk::result_ptr r;
    if ( q->numResults() )
        r = q->results().at( 0 );

    QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );
    bool found = true;

    foreach( const QString& s, sl )
    {
        if ( !r.isNull() )
        {
            if ( !r->artist()->name().contains( s, Qt::CaseInsensitive ) &&
                 !r->album()->name().contains( s, Qt::CaseInsensitive ) &&
                 !r->track() .contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }
        else
        {
            if ( !q->artist().contains( s, Qt::CaseInsensitive ) &&
                 !q->album() .contains( s, Qt::CaseInsensitive ) &&
                 !q->track() .contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }
    }

    return found;
}


void
TrackProxyModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( !sourceModel() )
        return;
    if ( !index.isValid() )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
TrackProxyModel::removeIndexes( const QModelIndexList& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() && idx.column() == 0 )
            pil << mapToSource( idx );
    }

    bool b = true;
    foreach( const QPersistentModelIndex& idx, pil )
    {
        if ( idx == pil.last() )
            b = false;

        qDebug() << "b is:" << b;
        sourceModel()->removeIndex( idx, b );
    }
}


void
TrackProxyModel::removeIndexes( const QList<QPersistentModelIndex>& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() && idx.column() == 0 )
            pil << mapToSource( idx );
    }

    bool b = true;
    foreach( const QPersistentModelIndex& idx, pil )
    {
        if ( idx == pil.last() )
            b = false;

        qDebug() << "b is:" << b;
        sourceModel()->removeIndex( idx, b );
    }
}