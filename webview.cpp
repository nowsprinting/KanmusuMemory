/*
 * Copyright 2013 KanMemo Project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "webview.h"

#include <QtCore/QDebug>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QMenu>
#include <QWebHitTestResult>
#include <QContextMenuEvent>

class WebView::Private
{
public:
    Private(WebView *parent);

private:
    void showNormal();
    void showFullScreen();
    void showOptionalSize(int width, int height, bool isfullscreen);

private:
    WebView *q;

public:
    ViewMode viewMode;

private:
    QHash<QString, QString> naviBar;
    QHash<QString, QString> naviApp;
    QHash<QString, QString> foot;
    QHash<QString, QString> body;
    QHash<QString, QString> gameFrame;
    QHash<QString, QString> flashWrap;
    QHash<QString, QString> embed;
    QHash<QString, QString> sectionWrap;
    QRect defaultRect;

    bool setElementProperty(QWebElement &element, QHash<QString, QString> &properties, QHash<QString, QString> &backup);
};

WebView::Private::Private(WebView *parent)
    : q(parent)
    , viewMode(NormalMode)
{
    //表示モードの変更
    connect(q, &WebView::viewModeChanged, [this](ViewMode viewMode) {
        switch (viewMode) {
        case WebView::NormalMode:
            showNormal();
            break;
        case WebView::FullScreenMode:
            showFullScreen();
            break;
        }
    });
    //ズームの変更
    connect(q, &WebView::gameSizeFactorChanged, [this](qreal factor) {
        if(!defaultRect.isValid()){
            defaultRect = q->getGameRect();
        }
        showOptionalSize(defaultRect.width() * factor, defaultRect.height() * factor, false);
    });
}

void WebView::Private::showFullScreen()
{
    //normal -> full
    showOptionalSize(q->window()->width(), q->window()->height(), true);
}

void WebView::Private::showOptionalSize(int width, int height, bool isfull)
{
    QWebFrame *frame = q->page()->mainFrame();

    ///////////////////////////////////////
    //スクロールバー非表示
    QWebElement element = frame->findFirstElement(QStringLiteral("body"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    QHash<QString, QString> properties;
    properties.insert(QStringLiteral("overflow"), QStringLiteral("hidden"));
    if(body.isEmpty()){
        foreach (const QString &key, properties.keys()) {
            body.insert(key, element.styleProperty(key, QWebElement::InlineStyle));
        }
        qDebug() << element.styleProperty(QStringLiteral("overflow"), QWebElement::InlineStyle);
    }
    if(isfull){
        //フルスクリーンのみ
        foreach (const QString &key, properties.keys()) {
            element.setStyleProperty(key, properties.value(key));
        }
    }
    properties.clear();

    /////////////////////////////////////////
    //ナビバーの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#dmm_ntgnavi"));
    properties.insert(QStringLiteral("width"), QString("%1px").arg(width+188));
    if(!setElementProperty(element, properties, naviBar)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();


    /////////////////////////////////////////
    //下のナビバーの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#area-game"));
    element = element.nextSibling();
    properties.insert(QStringLiteral("width"), QString("%1px").arg(width+188));
    if(!setElementProperty(element, properties, naviApp)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();

    /////////////////////////////////////////
    //フッターの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#foot"));
    properties.insert(QStringLiteral("width"), QString("%1px").arg(width+188));
    if(!setElementProperty(element, properties, foot)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();

    /////////////////////////////////////////
    //フレームを最大化
    element = frame->findFirstElement(QStringLiteral("#game_frame"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        //            ui.statusBar->showMessage(tr("failed find target"), STATUS_BAR_MSG_TIME);
        return;
    }
    if(!isfull){
        //フルスクリーンじゃない
        properties.insert(QStringLiteral("position"), QStringLiteral(""));
        properties.insert(QStringLiteral("top"), QStringLiteral(""));
        properties.insert(QStringLiteral("left"), QStringLiteral(""));
        properties.insert(QStringLiteral("width"), QString("%1px").arg(width+100));
        properties.insert(QStringLiteral("height"), QString("%1px").arg(height+100));
        properties.insert(QStringLiteral("z-index"), QStringLiteral(""));
    }else{
        properties.insert(QStringLiteral("position"), QStringLiteral("absolute"));
        properties.insert(QStringLiteral("top"), QStringLiteral("0px"));
        properties.insert(QStringLiteral("left"), QStringLiteral("0px"));
        properties.insert(QStringLiteral("width"), QString("%1px").arg(width+100));
        properties.insert(QStringLiteral("height"), QString("%1px").arg(height+100));
        properties.insert(QStringLiteral("z-index"), QStringLiteral("10"));
    }
    if(!setElementProperty(element, properties, gameFrame)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();


    /////////////////////////////////////////
    //フレームの子供からflashの入ったdivを探して、さらにその中のembedタグを調べる
    frame = frame->childFrames().first();
    element = frame->findFirstElement(QStringLiteral("#flashWrap"));
    if(!isfull){
        //フルスクリーンじゃない
        properties.insert(QStringLiteral("position"), QStringLiteral(""));
        properties.insert(QStringLiteral("top"), QStringLiteral(""));
        properties.insert(QStringLiteral("left"), QStringLiteral(""));
        properties.insert(QStringLiteral("width"), QString("%1px").arg(width));
        properties.insert(QStringLiteral("height"), QString("%1px").arg(height));
    }else{
        properties.insert(QStringLiteral("position"), QStringLiteral("absolute"));
        properties.insert(QStringLiteral("top"), QStringLiteral("0px"));
        properties.insert(QStringLiteral("left"), QStringLiteral("0px"));
        properties.insert(QStringLiteral("width"), QString("%1px").arg(width));
        properties.insert(QStringLiteral("height"), QString("%1px").arg(height));
    }
    if(!setElementProperty(element, properties, flashWrap)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();

    /////////////////////////////////////////
    //embedタグを探す
    element = element.findFirst(QStringLiteral("embed"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    properties.insert(QStringLiteral("width"), QString::number(width));
    properties.insert(QStringLiteral("height"), QString::number(height));
    if(embed.isEmpty()){
        foreach (const QString &key, properties.keys()) {
            embed.insert(key, element.attribute(key));
        }
        qDebug() << element.attribute(QStringLiteral("width"))
                 << "," << element.attribute(QStringLiteral("height"))
                 << "->" << width << "," << height;
    }
    foreach (const QString &key, properties.keys()) {
        element.evaluateJavaScript(QString("this.%1='%2'").arg(key).arg(properties.value(key)));
    }
    properties.clear();

    /////////////////////////////////////////
    //解説とか消す
    element = frame->findFirstElement(QStringLiteral("#sectionWrap"));
    if(!isfull){
        properties.insert(QStringLiteral("visibility"), QStringLiteral(""));
    }else{
        //フルスクリーンのみ
        properties.insert(QStringLiteral("visibility"), QStringLiteral("hidden"));
    }
    if(!setElementProperty(element, properties, sectionWrap)){
        qDebug() << "failed find target";
        return;
    }
    properties.clear();
}

//エレメントにプロパティを設定する（初回はバックアップする）
bool WebView::Private::setElementProperty(QWebElement &element, QHash<QString, QString> &properties, QHash<QString, QString> &backup)
{
    if(element.isNull()){
        qDebug() << "failed find target";
        return false;
    }
    QString text;
    if(backup.isEmpty()){
        foreach (const QString &key, properties.keys()) {
            backup.insert(key, element.styleProperty(key, QWebElement::InlineStyle));

            text.append(QString("%1(%2) ").arg(key).arg(element.styleProperty(key, QWebElement::InlineStyle)));
        }
        qDebug() << text;
    }
    foreach (const QString &key, properties.keys()) {
        if(properties.value(key).length() > 0){
            element.setStyleProperty(key, properties.value(key));
        }
    }

    return true;
}

void WebView::Private::showNormal()
{
    //full -> normal

    QWebFrame *frame = q->page()->mainFrame();

    /////////////////////////////////////////
    //スクロールバー表示
    QWebElement element = frame->findFirstElement(QStringLiteral("body"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, body.keys()) {
        element.setStyleProperty(key, body.value(key));
    }

    /////////////////////////////////////////
    //ナビバーの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#dmm_ntgnavi"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, naviBar.keys()) {
        element.setStyleProperty(key, naviBar.value(key));
    }

    /////////////////////////////////////////
    //下のナビバーの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#area-game"));
    element = element.nextSibling();
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, naviApp.keys()) {
        element.setStyleProperty(key, naviApp.value(key));
    }

    /////////////////////////////////////////
    //フッターの横幅をあわせる
    element = frame->findFirstElement(QStringLiteral("#foot"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, foot.keys()) {
        element.setStyleProperty(key, foot.value(key));
    }

    /////////////////////////////////////////
    //フレーム
    element = frame->findFirstElement(QStringLiteral("#game_frame"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, gameFrame.keys()) {
        element.setStyleProperty(key, gameFrame.value(key));
    }

    /////////////////////////////////////////
    //フレームの子供からflashの入ったdivを探して、さらにその中のembedタグを調べる
    frame = frame->childFrames().first();
    element = frame->findFirstElement(QStringLiteral("#flashWrap"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, flashWrap.keys()) {
        element.setStyleProperty(key, flashWrap.value(key));
    }

    /////////////////////////////////////////
    element = element.findFirst(QStringLiteral("embed"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        return;
    }
    //もとに戻す
    foreach (const QString &key, embed.keys()) {
        element.evaluateJavaScript(QStringLiteral("this.%1='%2'").arg(key).arg(embed.value(key)));
    }

    /////////////////////////////////////////
    //解説とか
    element = frame->findFirstElement(QStringLiteral("#sectionWrap"));
    if (element.isNull()) {
        qDebug() << "failed find target";
        //            ui.statusBar->showMessage(tr("failed find target"), STATUS_BAR_MSG_TIME);
        return;
    }
    //もとに戻す
    foreach (const QString &key, sectionWrap.keys()) {
        element.setStyleProperty(key, sectionWrap.value(key));
    }
}

WebView::WebView(QWidget *parent)
    : QWebView(parent)
    , d(new Private(this))
{
    setAttribute(Qt::WA_AcceptTouchEvents, false);
    connect(this, &QObject::destroyed, [this]() { delete d; });
}

bool WebView::gameExists() const
{
    return getGameRect().isValid();
}

//ゲームの領域を調べる
QRect WebView::getGameRect() const
{
    //スクロール位置は破壊される
    //表示位置を一番上へ強制移動
    page()->mainFrame()->setScrollPosition(QPoint(0, 0));
    //フレームを取得
    QWebFrame *frame = page()->mainFrame();
    if (frame->childFrames().isEmpty()) {
        return QRect();
    }
    //フレームの子供からflashの入ったdivを探して、さらにその中のembedタグを調べる
    frame = frame->childFrames().first();
    QWebElement element = frame->findFirstElement(QStringLiteral("#flashWrap"));
    if (element.isNull()) {
        return QRect();
    }
    element = element.findFirst(QStringLiteral("embed"));
    if (element.isNull()) {
        return QRect();
    }
    //見つけたタグの座標を取得
    QRect geometry = element.geometry();
    geometry.moveTopLeft(geometry.topLeft() + frame->geometry().topLeft());

    return geometry;
}

//ゲーム画面をキャプチャ
QImage WebView::capture()
{
    QImage ret;

    //スクロール位置の保存
    QPoint currentPos = page()->mainFrame()->scrollPosition();
    QRect geometry = getGameRect();
    if (!geometry.isValid()) {
        emit error(tr("failed find target"));
        goto finally;
    }

    {
        ret = QImage(geometry.size(), QImage::Format_ARGB32);
        QPainter painter(&ret);
        //全体を描画
        render(&painter, QPoint(0,0), geometry);
    }

finally:
    //スクロールの位置を戻す
    page()->mainFrame()->setScrollPosition(currentPos);
    return ret;
}

WebView::ViewMode WebView::viewMode() const
{
    return d->viewMode;
}

void WebView::setViewMode(ViewMode viewMode)
{
    if (d->viewMode == viewMode) return;
    d->viewMode = viewMode;
    emit viewModeChanged(viewMode);
}
//リンクをクリックした時のメニューをつくる
void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());
    if (!r.linkUrl().isEmpty()) {
        QMenu menu(this);
        menu.addAction(tr("Open in New Tab"), this, SLOT(openLinkInNewTab()));
        menu.addSeparator();
        menu.addAction(pageAction(QWebPage::CopyLinkToClipboard));
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
    QWebView::contextMenuEvent(event);
}
//タブで開くのトリガー
void WebView::openLinkInNewTab()
{
    pageAction(QWebPage::OpenLinkInNewWindow)->trigger();
}
//ゲームの画面サイズ
qreal WebView::getGameSizeFactor() const
{
    return gameSizeFactor;
}
//ゲームの画面サイズ
void WebView::setGameSizeFactor(const qreal &factor)
{
    if(gameSizeFactor == factor) return;
    if(factor < 0){
        gameSizeFactor = 0;
    }else{
        gameSizeFactor = factor;
    }
    emit gameSizeFactorChanged(factor);
}

