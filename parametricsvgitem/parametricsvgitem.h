#ifndef PARAMETRICSVGITEM_H
#define PARAMETRICSVGITEM_H


#include <QGraphicsSvgItem>
#include <QDomDocument>

class QJSEngine;

class ParametricSvgItem : public QGraphicsSvgItem
{
    Q_OBJECT
private:
    enum { Type = UserType + 845 };

    struct Parameter
    {
        QVariant value;        
        qreal min;
        qreal max;
        QString name;
    };

    struct Expression {
        QString name;
        QString value;
    };

    //Размеры компонента
    QMap<QString, Parameter> m_parameters;
    //Выражения JS
    QList<Expression> m_expressions;
    QDomDocument m_xmlDoc;
    QString m_namespace;
    QStringList m_errors;


    //методы
    bool readXmlFromFile(const QString &fname);
    bool readParameters(const QDomNode &node);
    bool readExpressions(const QDomNode &node);

    QString getUri(const QString &qName);
    QString getLocalName(const QString &qName);
    QString getToken(const QString &qName, const int index, const QString delimeter = ":");

    void addParameter(const QString &pName, const Parameter &param);
    void addExpression(const Expression &exp);

    void evaluateParameters(QJSEngine *jsEngine);
    void evaluateExpressions(QJSEngine *jsEngine);

    void evaluateXmlDocument(QJSEngine *jsEngine);
    void traverseXmlNode(const QDomNode &node, QJSEngine *jsEngine);
    bool domNodeIsValid(const QDomNode &node);
    Parameter domNodeToParameter(const QDomNode &node);
    Expression domNodeToExpression(const QDomNode &node);

    void evaluateAll();
    void redraw();

    void setNamespace(QString m) {m_namespace = m;}
    QString getNamespace() {return m_namespace;}

    void expressionValuesToParameterValues(QJSEngine *jsEngine);
    bool isNumberValueInRange(const Parameter &param, const QVariant value);

    void addError(bool isError, QString message);


public slots:
    void changeParamByName(const QString &pName, qreal d);

public:
    ParametricSvgItem(QGraphicsItem *parent = nullptr, const QString namespaceName = "parametric");
    ParametricSvgItem(const QString &fname, const QString namespaceName = "parametric", QGraphicsItem *parent = nullptr);
    ~ParametricSvgItem();

    int type() const override;
    bool setContent(const QString &fname);

    bool setParameter(const QString &pName, QVariant value);
    bool updateByParameter(const QString &pName, QVariant value);

    QVariant::Type parameterType(const QString &pName) const;
    QVariant parameterValue(const QString &pName) const;
    qreal parameterMin(const QString &pName) const;
    qreal parameterMax(const QString &pName) const;
    bool parameterIsExist(const QString &pName) const;
    QStringList parameterNames() const;
    int parametersCount();

    bool isError();
    QStringList &errors();
};

#endif // PARAMETRICSVGITEM_H
