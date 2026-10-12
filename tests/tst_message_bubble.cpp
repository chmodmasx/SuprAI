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

        // A component can instantiate successfully but render HTML markup
        // literally. Assert the actual QTextDocument content, not only QML
        // construction or the model's generated markup.
        QObject *prose = instance->findChild<QObject *>(QStringLiteral("proseText"));
        QTRY_VERIFY_WITH_TIMEOUT(prose, 3000);
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
    }
};

QTEST_MAIN(MessageBubbleTest)
#include "tst_message_bubble.moc"
