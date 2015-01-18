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

#pragma once
#ifndef FETCHMETADATA_H
#define FETCHMETADATA_H

#include <boost/shared_ptr.hpp>

#include <QtWidgets/QDialog>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ui_FetchMetadata.h"
#include "BookManipulation/Metadata.h"


class MetadataListItem
{
public:

    QString url;
    QString author;
    QString title;
    QString category;
    QString coverUrl;
    int rating;

    QString DisplayName() const;
};


class MetadataListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    MetadataListModel(const QList<MetadataListItem> &model, QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    QList<MetadataListItem> fetchedMetadata;
};


/**
 * The dialog for fetching metadata from external server.
 */
class FetchMetadata : public QDialog
{
    Q_OBJECT

public:

    FetchMetadata(const QString &title, const QString &author, QWidget *parent = 0);

signals:

private slots:

    void Search();

    void ParseNetworkResponse(QNetworkReply *finished);
    void CreateListModel(const QString &json);
    MetadataListItem DecodeMetadataListItem(const QJsonObject &jsonObj);

private:

    Ui::FetchMetadata ui;

    QNetworkAccessManager m_NetworkManager;

    boost::shared_ptr<QList<MetadataListItem>> m_MetadataList;
    boost::shared_ptr<MetadataListModel> m_MetadataListModel;

};

#endif // FETCHMETADATA_H


