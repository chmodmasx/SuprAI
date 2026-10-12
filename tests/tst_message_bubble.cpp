#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTextDocument>
#include <QScopedPointer>
#include <QTest>
#include <QVariantMap>

class MessageBubbleTest final : public QObject
{
    Q_OBJECT
private slots:
    void copyFeedbackChangesWithoutResizing()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_COPY_BUTTON_FILE)));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> button(component.createWithInitialProperties({
            {QStringLiteral("contentToCopy"), QStringLiteral("print('hola')")},
        }));
        QVERIFY2(button, qPrintable(component.errorString()));
        QCOMPARE(button->property("text").toString(), QStringLiteral("Copiar"));
        const int before = button->property("implicitWidth").toInt();
        QVERIFY(QMetaObject::invokeMethod(button.get(), "confirmCopy"));
        QCOMPARE(button->property("text").toString(), QStringLiteral("✓ Copiado"));
        QCOMPARE(button->property("implicitWidth").toInt(), before);
        QVERIFY(button->setProperty("contentToCopy", QStringLiteral("otro bloque")));
        QCOMPARE(button->property("text").toString(), QStringLiteral("Copiar"));
        QCOMPARE(button->property("implicitWidth").toInt(), before);
    }


    void constructsStructuredMarkdownComponents()
    {
        QQmlEngine engine;
        QQmlComponent tableComponent(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_MARKDOWN_TABLE_FILE)));
        QVERIFY2(tableComponent.isReady(), qPrintable(tableComponent.errorString()));
        const QVariantMap firstCell{{QStringLiteral("raw"), QStringLiteral("A")},
                                    {QStringLiteral("formatted"), QStringLiteral("<b>A</b>")}};
        const QVariantMap secondCell{{QStringLiteral("raw"), QStringLiteral("B")},
                                     {QStringLiteral("formatted"), QStringLiteral("B")}};
        const QVariantMap tableData{
            {QStringLiteral("kind"), QStringLiteral("table")},
            {QStringLiteral("raw"), QStringLiteral("| A | B |\n|---|---|\n| x | y |")},
            {QStringLiteral("headers"), QVariantList{firstCell, secondCell}},
            {QStringLiteral("rows"), QVariantList{
                QVariantList{
                    QVariantMap{{QStringLiteral("formatted"), QStringLiteral("x")}},
                    QVariantMap{{QStringLiteral("formatted"), QStringLiteral("y")}}}}},
            {QStringLiteral("alignment"), QVariantList{QStringLiteral("left"), QStringLiteral("right")}}
        };
        QScopedPointer<QObject> table(tableComponent.createWithInitialProperties({
            {QStringLiteral("tableData"), tableData}
        }));
        QVERIFY2(table, qPrintable(tableComponent.errorString()));
        auto *tableItem = qobject_cast<QQuickItem *>(table.get());
        QVERIFY(tableItem);
        tableItem->setWidth(550);
        QTRY_VERIFY_WITH_TIMEOUT(tableItem->implicitHeight() > 45, 3000);
        QVERIFY(tableComponent.errors().isEmpty());

        QQmlComponent quoteComponent(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_MARKDOWN_QUOTE_FILE)));
        QVERIFY2(quoteComponent.isReady(), qPrintable(quoteComponent.errorString()));
        QScopedPointer<QObject> quote(quoteComponent.createWithInitialProperties({
            {QStringLiteral("formatted"), QStringLiteral("Una <b>cita</b> segura")}
        }));
        QVERIFY2(quote, qPrintable(quoteComponent.errorString()));
        auto *quoteItem = qobject_cast<QQuickItem *>(quote.get());
        QVERIFY(quoteItem);
        quoteItem->setWidth(550);
        QTRY_VERIFY_WITH_TIMEOUT(quoteItem->implicitHeight() > 30, 3000);

        QQmlComponent bubbleComponent(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_MESSAGE_BUBBLE_FILE)));
        QVERIFY2(bubbleComponent.isReady(), qPrintable(bubbleComponent.errorString()));
        const QVariantMap initial{
            {QStringLiteral("itemId"), QStringLiteral("table-quote")},
            {QStringLiteral("speaker"), QStringLiteral("assistant")},
            {QStringLiteral("text"), tableData.value("raw")},
            {QStringLiteral("displayText"), QStringLiteral("formatted")},
            {QStringLiteral("displayStyled"), true},
            {QStringLiteral("streaming"), false},
            {QStringLiteral("messageState"), QStringLiteral("completed")},
            {QStringLiteral("messageBlocks"), QVariantList{
                tableData,
                QVariantMap{{QStringLiteral("kind"), QStringLiteral("quote")},
                            {QStringLiteral("formatted"), QStringLiteral("Una cita")}}
            }}
        };
        QScopedPointer<QObject> bubble(bubbleComponent.createWithInitialProperties(initial));
        QVERIFY2(bubble, qPrintable(bubbleComponent.errorString()));
        auto *bubbleItem = qobject_cast<QQuickItem *>(bubble.get());
        QVERIFY(bubbleItem);
        bubbleItem->setWidth(900);
        QTRY_VERIFY_WITH_TIMEOUT(bubbleItem->implicitHeight() > 80, 3000);
        QVERIFY(bubbleComponent.errors().isEmpty());
    }

    void constructsAndLaysOutSafeTextAndCode()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_MESSAGE_BUBBLE_FILE)));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        QVariantList segments{
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("text")},
                        {QStringLiteral("formatted"), QStringLiteral("Hola <b>mundo</b><br/>Un &amp; dos <i>cursiva</i>")}},
            QVariantMap{{QStringLiteral("kind"), QStringLiteral("code")},
                        {QStringLiteral("language"), QStringLiteral("cpp")},
                        {QStringLiteral("raw"), QStringLiteral("int answer = 42;")}},
        };
        const QVariantMap initial{
            {QStringLiteral("itemId"), QStringLiteral("fixture-assistant")},
            {QStringLiteral("speaker"), QStringLiteral("assistant")},
            {QStringLiteral("text"), QStringLiteral("Hola mundo, int answer = 42;")},
            {QStringLiteral("displayText"), QStringLiteral("Hola <b>mundo</b>")},
            {QStringLiteral("displayStyled"), true},
            {QStringLiteral("streaming"), false},
            {QStringLiteral("messageState"), QStringLiteral("completed")},
            {QStringLiteral("messageBlocks"), segments},
        };

        QScopedPointer<QObject> instance(component.createWithInitialProperties(initial));
        QVERIFY2(instance, qPrintable(component.errorString()));
        auto *item = qobject_cast<QQuickItem *>(instance.get());
        QVERIFY(item);
        item->setWidth(950);
        QTRY_VERIFY_WITH_TIMEOUT(item->implicitHeight() > 30, 3000);
        QVERIFY2(component.errors().isEmpty(), qPrintable(component.errorString()));

        // Probe the exact selectable prose component independently rather than
        // relying on private QObject parenting of Repeater/Loader children.
        QQmlComponent proseComponent(&engine, QUrl::fromLocalFile(
            QStringLiteral(SUPRAI_PROSE_TEXT_FILE)));
        QVERIFY2(proseComponent.isReady(), qPrintable(proseComponent.errorString()));

        QScopedPointer<QObject> prose(proseComponent.createWithInitialProperties({
            {QStringLiteral("formatted"),
             QStringLiteral("Hola <b>mundo</b><br/>Un &amp; dos <i>cursiva</i>")},
            {QStringLiteral("styled"), true},
        }));
        QVERIFY2(prose, qPrintable(proseComponent.errorString()));
        QCOMPARE(prose->property("textFormat").toInt(), int(Qt::RichText));
        const auto quickDocument =
            prose->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(quickDocument);
        const QString rendered = quickDocument->textDocument()->toPlainText();
        QCOMPARE(rendered, QStringLiteral("Hola mundo\nUn & dos cursiva"));
        QVERIFY(!rendered.contains(QStringLiteral("<b>")));
        QVERIFY(!rendered.contains(QStringLiteral("<br")));
        QVERIFY(prose->property("readOnly").toBool());
        QVERIFY(prose->property("selectByMouse").toBool());

        QScopedPointer<QObject> styledStreaming(proseComponent.createWithInitialProperties({
            {QStringLiteral("formatted"), QStringLiteral("Hola <b>mundo</b>")},
            {QStringLiteral("styled"), true},
            {QStringLiteral("streaming"), true},
        }));
        QVERIFY2(styledStreaming, qPrintable(proseComponent.errorString()));
        QCOMPARE(styledStreaming->property("textFormat").toInt(), int(Qt::RichText));
        auto *previewDoc = styledStreaming->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(previewDoc);
        QCOMPARE(previewDoc->textDocument()->toPlainText(), QStringLiteral("Hola mundo ▋"));

        QScopedPointer<QObject> streaming(proseComponent.createWithInitialProperties({
            {QStringLiteral("formatted"), QStringLiteral("**sin procesar**")},
            {QStringLiteral("styled"), false},
            {QStringLiteral("streaming"), true},
        }));
        QVERIFY2(streaming, qPrintable(proseComponent.errorString()));
        QCOMPARE(streaming->property("textFormat").toInt(), int(Qt::PlainText));
        auto *streamDoc = streaming->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(streamDoc);
        QCOMPARE(streamDoc->textDocument()->toPlainText(),
                 QStringLiteral("**sin procesar** ▋"));
    }
};

QTEST_MAIN(MessageBubbleTest)
#include "tst_message_bubble.moc"
