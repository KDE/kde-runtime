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


#include "transactionmodel.h"

#include <Soprano/Statement>
#include <Soprano/StatementIterator>

namespace Nepomuk {

TransactionModel::TransactionModel(Soprano::Model* parent)
    : FilterModel(parent)
{
    connect( this, SIGNAL(statementAdded(Soprano::Statement)),
             this, SLOT(recordAddStatement(Soprano::Statement)) );
    connect( this, SIGNAL(statementRemoved(Soprano::Statement)),
             this, SLOT(recordRemoveAllStatements(Soprano::Statement)) );
}

TransactionModel::~TransactionModel()
{
    roleback();
}

void TransactionModel::commit()
{
    m_added.clear();
    m_removed.clear();
}

void TransactionModel::roleback()
{
    foreach( const Soprano::Statement & st, m_added ) {
        Soprano::FilterModel::removeStatement( st );
    }

    foreach( const Soprano::Statement & st, m_removed ) {
        Soprano::FilterModel::addStatement( st );
    }

    m_added.clear();
    m_removed.clear();
}

Soprano::Error::ErrorCode TransactionModel::addStatement(const Soprano::Statement& statement)
{
    recordAddStatement( statement );
    return Soprano::FilterModel::addStatement(statement);
}

Soprano::Error::ErrorCode TransactionModel::removeStatement(const Soprano::Statement& statement)
{
    return Soprano::FilterModel::removeStatement(statement);
}

Soprano::Error::ErrorCode TransactionModel::removeAllStatements(const Soprano::Statement& statement)
{
    recordRemoveAllStatements( statement );
    return Soprano::FilterModel::removeAllStatements(statement);
}

// Slots
void TransactionModel::recordAddStatement(const Soprano::Statement& st)
{
    m_added.insert( st );
}

void TransactionModel::recordRemoveAllStatements(const Soprano::Statement& st)
{
    QList< Soprano::Statement > stList = listStatements( st).allStatements();
    m_removed += stList.toSet();
}

void TransactionModel::recordRemoveStatement(const Soprano::Statement& st)
{
    m_removed.insert( st );
}

}