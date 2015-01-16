/************************************************************************
**
**  Copyright (C) 2015 Marek Gibek
**
**  This file is part of Sigil.
**
**  Sigil is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Sigil is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Sigil.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#include <QtCore/QDate>
#include <QtCore/QModelIndex>

#include <QUrl>

#include "BookManipulation/Metadata.h"
#include "Dialogs/FetchMetadata.h"

FetchMetadata::FetchMetadata(const QString &title, const QString &author, QWidget *parent)
    :
    QDialog(parent)
{
    ui.setupUi(this);

    if (!title.isEmpty()) {
        ui.editFilter->setText(title);
    } else {
        ui.editFilter->setText(author);
    }

    connect(ui.btSearch, SIGNAL(clicked()), this, SLOT(Search()));
}

//
//
//
void FetchMetadata::Search()
{
    QString searchText = ui.editFilter->text();

    QString url = "http://lubimyczytac.pl/searcher/getsuggestions?phrase=";
    url += QUrl::toPercentEncoding(searchText);
}
