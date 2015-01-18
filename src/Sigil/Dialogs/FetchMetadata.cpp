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
#include <QtWidgets/QMessageBox>

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QStringListModel>
#include <QUrl>

#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QJsonArray>

#include "BookManipulation/Metadata.h"
#include "Dialogs/FetchMetadata.h"

using boost::shared_ptr;


//
//
//

QString MetadataListItem::DisplayName() const
{
    return QString("%1 - %2").arg(author).arg(title);
}


//
//
//

MetadataListModel::MetadataListModel(const QList<MetadataListItem> &model, QObject *parent)
    : QAbstractListModel(parent)
{
    fetchedMetadata = model;
}

int MetadataListModel::rowCount(const QModelIndex &parent) const
{
    return fetchedMetadata.size();
}

QVariant MetadataListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() >= fetchedMetadata.size()) return QVariant();

    if(role == Qt::DisplayRole ) {
        return QVariant(fetchedMetadata.at(index.row()).DisplayName());
    }
    else {
        return QVariant();
    }
}


//
//
//

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
    connect(&m_NetworkManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(ParseNetworkResponse(QNetworkReply*)));
}

//
//
//
void FetchMetadata::Search()
{
    QString searchText = ui.editFilter->text();

    QString url = "http://lubimyczytac.pl/searcher/getsuggestions?phrase=";
    url += QUrl::toPercentEncoding(searchText);
    QUrl qurl(url);

    QNetworkRequest request;
    request.setUrl(qurl);
    m_NetworkManager.get(request);
}

void FetchMetadata::ParseNetworkResponse(QNetworkReply *finished)
{
    if (finished->error() != QNetworkReply::NoError)
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Error code: %1").arg(finished->error())));
        return;
    }

    CreateListModel((QString) finished->readAll());
    ui.lvResults->setModel(m_MetadataListModel.get());
}

void FetchMetadata::CreateListModel(const QString &json)
{
    m_MetadataList.reset(new QList<MetadataListItem>());

    QJsonDocument jsonResponse = QJsonDocument::fromJson(json.toUtf8());
    if (jsonResponse.isArray()) {
        QJsonArray jsonArr = jsonResponse.array();
        foreach (const QJsonValue & value, jsonArr) {
            QJsonObject obj = value.toObject();
            MetadataListItem metadata = DecodeMetadataListItem(obj);
            if (metadata.category == "book") {
                m_MetadataList->append(metadata);
            }
        }
    } else if (jsonResponse.isObject()) {
        QJsonObject jsonObj = jsonResponse.object();
        foreach (const QJsonValue & value, jsonObj) {
            QJsonObject obj = value.toObject();
            MetadataListItem metadata = DecodeMetadataListItem(obj);
            if (metadata.category == "book") {
                m_MetadataList->append(metadata);
            }
        }
    }

    m_MetadataListModel.reset(new MetadataListModel(*m_MetadataList));
}

MetadataListItem FetchMetadata::DecodeMetadataListItem(const QJsonObject &jsonObj)
{
    MetadataListItem metadata;
    metadata.url = jsonObj["url"].toString();
    metadata.title = jsonObj["title"].toString();
    metadata.category = jsonObj["category"].toString();
    metadata.coverUrl = jsonObj["cover"].toString();
    metadata.rating = jsonObj["rating"].toInt();
    metadata.author = "";
    QJsonArray jsonArr = jsonObj["authors"].toArray();
    foreach (const QJsonValue & value, jsonArr) {
        QJsonObject obj = value.toObject();
        QString fullname = obj["fullname"].toString();
        if (!metadata.author.isEmpty()) {
            metadata.author += ", ";
        }
        metadata.author += fullname;
    }
    return metadata;
}
