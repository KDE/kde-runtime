/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef TRANSACTIONMODEL_H
#define TRANSACTIONMODEL_H

#include <QtCore/QSet>
#include <Soprano/FilterModel>

namespace Soprano {
    class Statement;
}

namespace Nepomuk {

    /**
     * This model represents one transaction. It does so by simply recording
     * all the added and removed statements and reverting all the changing
     * on destruction, unless commit() has been called.
     *
     */
    class TransactionModel : public Soprano::FilterModel
    {
        Q_OBJECT
    public:
        TransactionModel(Model* parent);
        virtual ~TransactionModel();

        virtual Soprano::Error::ErrorCode addStatement(const Soprano::Statement& statement);
        
        virtual Soprano::Error::ErrorCode removeAllStatements(const Soprano::Statement& statement);
        virtual Soprano::Error::ErrorCode removeStatement(const Soprano::Statement& statement);

        void commit();
        void roleback();

    private slots:
        void recordAddStatement( const Soprano::Statement & st );
        void recordRemoveStatement( const Soprano::Statement & st );
        void recordRemoveAllStatements( const Soprano::Statement & st );
        
    private:
        QSet<Soprano::Statement> m_added;
        QSet<Soprano::Statement> m_removed;
    };

}

#endif // TRANSACTIONMODEL_H
