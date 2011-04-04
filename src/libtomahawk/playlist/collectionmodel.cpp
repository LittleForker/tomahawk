/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "collectionmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "sourcelist.h"
#include "utils/tomahawkutils.h"

using namespace Tomahawk;


CollectionModel::CollectionModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( 0 )
{
    qDebug() << Q_FUNC_INFO;

    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceOffline( Tomahawk::source_ptr ) ) );
}


CollectionModel::~CollectionModel()
{
//    delete m_rootItem;
}


QModelIndex
CollectionModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    PlItem* parentItem = itemFromIndex( parent );
    PlItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
CollectionModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    PlItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
CollectionModel::columnCount( const QModelIndex& parent ) const
{
    return 4;
}


QModelIndex
CollectionModel::parent( const QModelIndex& child ) const
{
    PlItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    PlItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    PlItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
CollectionModel::data( const QModelIndex& index, int role ) const
{
    if ( role != Qt::DisplayRole )
        return QVariant();

    PlItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    const query_ptr& query = entry->query();
    if ( query.isNull() )
    {
        if ( !index.column() )
        {
            return entry->caption.isEmpty() ? "Unknown" : entry->caption;
        }

        if ( index.column() == 1 )
        {
            return entry->childCount;
        }

        return QVariant( "" );
    }

    if ( !query->numResults() )
    {
        switch( index.column() )
        {
            case 0:
                return query->track();
                break;
        }
    }
    else
    {
        switch( index.column() )
        {
            case 0:
                return query->results().first()->track();
                break;

            case 1:
                return QVariant();
                break;

            case 2:
                return TomahawkUtils::timeToString( query->results().first()->duration() );
                break;

            case 3:
                return query->results().first()->collection()->source()->friendlyName();
                break;
        }
    }

    return QVariant();
}


QVariant
CollectionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Name" ) << tr( "Tracks" ) << tr( "Duration" ) << tr( "Origin" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


void
CollectionModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    emit loadingStarted();

    connect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                                  SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );
    connect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                                  SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );
}


void
CollectionModel::removeCollection( const collection_ptr& collection )
{
    disconnect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );
    disconnect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );

    QList<PlItem*> plitems = m_collectionIndex.values( collection );

    m_collectionIndex.remove( collection );
}


void
CollectionModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const collection_ptr& collection )
{
//    int c = rowCount( QModelIndex() );

    PlItem* plitem;
    foreach( const Tomahawk::query_ptr& query, tracks )
    {
        PlItem* parent = m_rootItem;
        if ( parent->hash.contains( query->artist() ) )
        {
            parent = parent->hash.value( query->artist() );
        }
        else
        {
            parent = new PlItem( query->artist(), m_rootItem );
            m_rootItem->hash.insert( query->artist(), parent );
        }

        if ( parent->hash.contains( query->album() ) )
        {
            parent->childCount++;
            parent = parent->hash.value( query->album() );
            parent->childCount++;
        }
        else
        {
            PlItem* subitem = new PlItem( query->album(), parent );
            parent->hash.insert( query->album(), subitem );
            parent->childCount++;
            subitem->childCount++;
            parent = subitem;
        }

        plitem = new PlItem( query, parent );
        m_collectionIndex.insertMulti( collection, plitem );
    }

    reset();

    qDebug() << rowCount( QModelIndex() );
}


void
CollectionModel::onTracksAddingFinished( const Tomahawk::collection_ptr& collection )
{
    qDebug() << "Finished loading tracks" << collection->source()->friendlyName();

    disconnect( collection.data(), SIGNAL( tracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ) );
    disconnect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );

    emit loadingFinished();
}


void
CollectionModel::onSourceOffline( Tomahawk::source_ptr src )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_collectionIndex.contains( src->collection() ) )
    {
        removeCollection( src->collection() );
    }
}


PlItem*
CollectionModel::itemFromIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
        return static_cast<PlItem*>( index.internalPointer() );
    else
    {
        return m_rootItem;
    }
}
