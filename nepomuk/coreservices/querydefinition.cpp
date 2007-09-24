/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "querydefinition.h"
#include <QtCore/QString>


const QString QueryDefinition::FIND_GRAPHS =
"select ?graphId ?modelId where { "
"?graphId <http://www.w3.org/2000/01/rdf-schema#label> ?modelId . "
"?graphId <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.soprano.org/types#Model> }";

// Resolve a uri to another
const QString QueryDefinition::TO_UNIQUE_URI = "select ?uri where (<@param>, <http://nepomuk.org/kde/ontology#hasUri>, ?uri)";
const QString QueryDefinition::FROM_UNIQUE_URI = "select ?uri where (?uri, <http://nepomuk.org/kde/ontology#hasUri>, <@param>)";
