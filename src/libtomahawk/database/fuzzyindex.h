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

#ifndef FUZZYINDEX_H
#define FUZZYINDEX_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QString>
#include <QMutex>

namespace lucene
{
    namespace analysis
    {
      class SimpleAnalyzer;
    }
    namespace store
    {
      class Directory;
    }
    namespace index
    {
      class IndexReader;
      class IndexWriter;
    }
    namespace search
    {
      class IndexSearcher;
    }
}

class DatabaseImpl;

class FuzzyIndex : public QObject
{
Q_OBJECT

public:
    explicit FuzzyIndex( DatabaseImpl& db, bool wipeIndex = false );
    ~FuzzyIndex();

    void beginIndexing();
    void endIndexing();
    void appendFields( const QString& table, const QMap< unsigned int, QString >& fields );
    
signals:
    void indexReady();

public slots:
    void loadLuceneIndex();

    QMap< int, float > search( const QString& table, const QString& name );

private:
    DatabaseImpl& m_db;
    QMutex m_mutex;
    QString m_lucenePath;

    lucene::analysis::SimpleAnalyzer* m_analyzer;
    lucene::store::Directory* m_luceneDir;
    lucene::index::IndexReader* m_luceneReader;
    lucene::search::IndexSearcher* m_luceneSearcher;
};

#endif // FUZZYINDEX_H
