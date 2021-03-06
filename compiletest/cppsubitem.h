#ifndef __CPP_SUBITEM__
#define __CPP_SUBITEM__

#include <QObject>
#include <QColor>

class CppSubItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    CppSubItem(QObject *parent = 0);

    QString name() const;
    void setName(const QString &name);

    QColor color() const;
    void setColor(const QColor &color);

private:
    QString m_name;
    QColor m_color;
};

#endif
