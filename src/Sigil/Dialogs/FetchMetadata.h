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

#include <QModelIndex>

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


class MetadataBookItem
{
public:

    QString coverUrl;
    QString description;
    QString category;
    QString series;
    int seriesIndex;
    double ratingValue;
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


class MetadataResult
{
public:

    bool bCover;
    QString coverUrl;
    bool bAuthor;
    QString author;
    bool bTitle;
    QString title;
    bool bCategory;
    QString category;
    bool bSeries;
    QString series;
    bool bSeriesIndex;
    QString seriesIndex;
    bool bRating;
    QString rating;
    bool bDescription;
    QString description;
};


/**
 * The dialog for fetching metadata from external server.
 */
class FetchMetadata : public QDialog
{
    Q_OBJECT

public:

    FetchMetadata(const QString &title, const QString &author, QWidget *parent = 0);

    MetadataResult GetResult();

signals:

private slots:

    void Search();
    void ListItemClicked(const QModelIndex &index);

    void ParseListResponse(QNetworkReply *finished);
    void ParseBookResponse(QNetworkReply *finished);

private:

    Ui::FetchMetadata ui;

    QString m_Description;
    QString m_Url;
    QString m_CoverUrl;

    QNetworkAccessManager m_NetworkListManager;
    QNetworkAccessManager m_NetworkBookManager;

    boost::shared_ptr<QList<MetadataListItem>> m_MetadataList;
    boost::shared_ptr<MetadataListModel> m_MetadataListModel;

    void CreateListModel(const QString &json);
    MetadataListItem DecodeMetadataListItem(const QJsonObject &jsonObj);
    MetadataBookItem DecodeMetadataBookItem(const QString &html);
    void PreprocessMetadataBookItem(MetadataBookItem &item);

    void DescriptionSetEnabled(bool enabled);
};

#endif // FETCHMETADATA_H


