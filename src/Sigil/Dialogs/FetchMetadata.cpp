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

#include <QWebFrame>

#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QJsonArray>

#include <QRegExp>

#include "BookManipulation/Metadata.h"
#include "Dialogs/FetchMetadata.h"
#include "sigil_constants.h"

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

    ui.lvResults->setEnabled(false);
    DescriptionSetEnabled(false);

    connect(ui.btSearch, SIGNAL(clicked()), this, SLOT(Search()));
    connect(ui.lvResults, SIGNAL(clicked(const QModelIndex &)), this, SLOT(ListItemClicked(const QModelIndex &)));

    connect(&m_NetworkListManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(ParseListResponse(QNetworkReply*)));
    connect(&m_NetworkBookManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(ParseBookResponse(QNetworkReply*)));
}

MetadataResult FetchMetadata::GetResult()
{
    MetadataResult result;

    result.bCover = ui.cbCover->isChecked();
    result.coverUrl = ui.labelCoverUrl->text();
    result.bAuthor = ui.cbAuthor->isChecked();
    result.author = ui.editAuthor->text();
    result.bTitle = ui.cbTitle->isChecked();
    result.title = ui.editTitle->text();
    result.bCategory = ui.cbCategory->isChecked();
    result.category = ui.editCategory->text();
    result.bSeries = ui.cbSeries->isChecked();
    result.series = ui.editSeries->text();
    result.bSeriesIndex = ui.cbSeriesIndex->isChecked();
    result.seriesIndex = QString::number(ui.sbSeriesIndex->value());
    result.bRating = ui.cbRating->isChecked();
    result.rating = QString::number(ui.spinRating->value(), 'f', 2);
    result.bDescription = ui.cbDescription->isChecked();
    result.description = m_Description;

    return result;
}

void FetchMetadata::Search()
{
    QString searchText = ui.editFilter->text();

    QString url = "http://lubimyczytac.pl/searcher/getsuggestions?phrase=";
    url += QUrl::toPercentEncoding(searchText);
    QUrl qurl(url);

    QNetworkRequest request;
    request.setUrl(qurl);
    m_NetworkListManager.get(request);

    ui.lvResults->setEnabled(false);
    DescriptionSetEnabled(false);

}

void FetchMetadata::ListItemClicked(const QModelIndex &index)
{
    int pos = index.row();
    if (pos < 0 || pos >= m_MetadataList->length()) {
        return;
    }

    MetadataListItem item = m_MetadataList->at(pos);

    DescriptionSetEnabled(false);
    ui.labelUrl->setText(item.url);
    ui.cbAuthor->setChecked(true);
    ui.editAuthor->setText(item.author);
    ui.cbTitle->setChecked(true);
    ui.editTitle->setText(item.title);

    QNetworkRequest request;
    request.setUrl(item.url);
    m_NetworkBookManager.get(request);
}

void FetchMetadata::ParseListResponse(QNetworkReply *finished)
{
    ui.lvResults->setEnabled(true);

    if (finished->error() != QNetworkReply::NoError)
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Error code: %1").arg(finished->error())));
        return;
    }

    CreateListModel((QString) finished->readAll());
    ui.lvResults->setModel(m_MetadataListModel.get());
}

void FetchMetadata::ParseBookResponse(QNetworkReply *finished)
{
    if (finished->error() != QNetworkReply::NoError)
    {
        QMessageBox::critical(this, tr("Error"), QString(tr("Error code: %1").arg(finished->error())));
        return;
    }

    MetadataBookItem bookItem = DecodeMetadataBookItem((QString) finished->readAll());

    ui.cbCover->setChecked(true);
    ui.labelCoverUrl->setText(bookItem.coverUrl);
    ui.webViewCover->setHtml(IMAGE_HTML_BASE_PREVIEW.arg(bookItem.coverUrl), QUrl(bookItem.coverUrl));
    ui.cbDescription->setChecked(true);
    ui.webViewDescription->setHtml(bookItem.description);
    ui.cbCategory->setChecked(true);
    ui.editCategory->setText(bookItem.category);
    ui.cbSeries->setChecked(!bookItem.series.isEmpty());
    ui.cbSeriesIndex->setChecked(!bookItem.series.isEmpty());
    ui.editSeries->setText(bookItem.series);
    ui.sbSeriesIndex->setValue(bookItem.seriesIndex);
    ui.cbRating->setChecked(true);
    ui.spinRating->setValue(bookItem.ratingValue);

    DescriptionSetEnabled(true);
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

MetadataBookItem FetchMetadata::DecodeMetadataBookItem(const QString &html)
{
    MetadataBookItem item;

    QRegExp rxCoverUrl("<div class=\"book-cover-size.*<a href=\"(.*)\"");
    rxCoverUrl.setMinimal(true);
    if (rxCoverUrl.indexIn(html, 0) != -1) {
        item.coverUrl = rxCoverUrl.cap(1);
    }

    QString descriptionShort;
    QRegExp rxDescriptionShort("<span id=\"sBookDescriptionShort\">(.*)</span>");
    rxDescriptionShort.setMinimal(true);
    if (rxDescriptionShort.indexIn(html, 0) != -1) {
        descriptionShort = rxDescriptionShort.cap(1);
    }

    QString descriptionLong;
    QRegExp rxDescriptionLong("<div id=\"sBookDescriptionLong\">(.*)</div>");
    rxDescriptionLong.setMinimal(true);
    if (rxDescriptionLong.indexIn(html, 0) != -1) {
        descriptionLong = rxDescriptionLong.cap(1);
    }

    item.description = !descriptionLong.isEmpty() ? descriptionLong : descriptionShort;

    QRegExp rxCategory("<dt>kategoria</dt>.*<dd><a.*>(.*)</a>");
    rxCategory.setMinimal(true);
    if (rxCategory.indexIn(html, 0) != -1) {
        item.category = rxCategory.cap(1).replace("/", ", ");
    }

    item.seriesIndex = 0;
    QRegExp rxSeries("Cykl: <a.*>(.*)</a>(.*)<");
    rxSeries.setMinimal(true);
    if (rxSeries.indexIn(html, 0) != -1) {
        item.series = rxSeries.cap(1);
        QString seriesIndex = rxSeries.cap(2);

        QRegExp rxSeriesIndex("tom (\\d+)");
        if (rxSeriesIndex.indexIn(seriesIndex, 0) != -1) {
            item.seriesIndex = rxSeriesIndex.cap(1).toInt();
        }
    }

    item.ratingValue = 0.0;
    QRegExp rxRating("<meta property=\"books:rating:value\".*content=\"(.*)\"");
    rxRating.setMinimal(true);
    if (rxRating.indexIn(html, 0) != -1) {
        item.ratingValue = rxRating.cap(1).toDouble();
    }

    PreprocessMetadataBookItem(item);

    return item;
}

void FetchMetadata::PreprocessMetadataBookItem(MetadataBookItem &item)
{
    m_Description = QString("<b>%1 / 10.0</b><br/><br/>%2<p><a href=\"%3\"/>%3</p>")
            .arg(QString::number(item.ratingValue, 'f', 2))
            .arg(item.description)
            .arg(ui.labelUrl->text());

    item.description = m_Description;
}

void FetchMetadata::DescriptionSetEnabled(bool enabled)
{
    ui.cbCover->setEnabled(enabled);
    ui.webViewCover->setEnabled(enabled);
    ui.cbAuthor->setEnabled(enabled);
    ui.editAuthor->setEnabled(enabled);
    ui.cbTitle->setEnabled(enabled);
    ui.editTitle->setEnabled(enabled);
    ui.cbCategory->setEnabled(enabled);
    ui.editCategory->setEnabled(enabled);
    ui.cbSeries->setEnabled(enabled);
    ui.editSeries->setEnabled(enabled);
    ui.cbSeriesIndex->setEnabled(enabled);
    ui.sbSeriesIndex->setEnabled(enabled);
    ui.cbRating->setEnabled(enabled);
    ui.spinRating->setEnabled(enabled);
    ui.cbDescription->setEnabled(enabled);
    ui.webViewDescription->setEnabled(enabled);
    ui.labelRatingMax->setEnabled(enabled);
}
