#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
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
                        {QStringLiteral("formatted"), QStringLiteral("Hola <b>mundo</b>")}},
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
    }
};

QTEST_MAIN(MessageBubbleTest)
#include "tst_message_bubble.moc"
