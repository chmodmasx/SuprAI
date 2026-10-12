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
