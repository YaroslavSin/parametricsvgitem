/*!
 * \class parametricsvgitem
 * Parametric SVG graphics item
 */
#include "parametricsvgitem.h"
#include <QFile>
#include <QDomDocument>
#include <QJSEngine>
#include <QXmlStreamReader>
#include <QSvgRenderer>

ParametricSvgItem::ParametricSvgItem(const QString &fname, const QString namespaceName, QGraphicsItem *parent):
    ParametricSvgItem::ParametricSvgItem(parent, namespaceName)
{
    setContent(fname);
}

ParametricSvgItem::ParametricSvgItem(QGraphicsItem *parent, const QString namespaceName):
    QGraphicsSvgItem::QGraphicsSvgItem(parent)
{
    setFlags(
                QGraphicsItem::ItemIsSelectable
                //| QGraphicsItem::ItemIsMovable
                | QGraphicsItem::ItemSendsGeometryChanges);

    QSvgRenderer *renderer = new QSvgRenderer();
    this->setSharedRenderer(renderer);

    setNamespace(namespaceName);

}

ParametricSvgItem::~ParametricSvgItem()
{
    if(renderer())
        delete renderer();
}

/*!
  * Load content from SVG file and evaluate parameters
  *
  * \param[in] fname Full path to SVG file
  * \return true on succes
  */
bool ParametricSvgItem::setContent(const QString &fname)
{

    bool isOk = readXmlFromFile(fname);
    if(!isOk){
        return false;
    }

    QDomElement docElem = m_xmlDoc.documentElement();
    if(docElem.isNull()){
        return false;
    }

    QDomNode defsNode = docElem.firstChildElement("defs");

    isOk = readParameters(defsNode);
    if(!isOk){
        return false;
    }

    //Выражения JS
    isOk = readExpressions(defsNode);
    if(!isOk){
        return false;
    }

    evaluateAll();
    redraw();
    return true;
}

/*!
  * Update graphics from SVG data
  */
void ParametricSvgItem::redraw()
{
    prepareGeometryChange();
    QXmlStreamReader xmlreader(m_xmlDoc.toString());
    this->renderer()->load(&xmlreader);
    this->setElementId("");
}

/*!
  * Load content from SVG file
  *
  * \param[in] fname Full path to SVG file
  * \return true on succes
  */
bool ParametricSvgItem::readXmlFromFile(const QString &fname)
{
    if(fname.isEmpty()){
        return false;
    }

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    if (!m_xmlDoc.setContent(&file)) {
        file.close();
        return false;
    }
    file.close();
    return true;
}

/*!
  * Read parameters from SVG and create it
  *
  * \param[in] node XML node with parameters declaration
  * \return true on succes
  */
bool ParametricSvgItem::readParameters(const QDomNode &node)
{
    if(node.isNull()){
        return false;
    }

    QDomNodeList list = node.toElement().elementsByTagName(getNamespace() + ":default");
    for(int i=0; i<list.size(); ++i){
        Parameter param = domNodeToParameter(list.at(i));

        if (!param.name.isEmpty()) {
            addParameter(param.name, param);
        }
    }
    return true;
}

/*!
  * Add variable parameter to parameters list
  *
  * \param[in] pName parameter name
  * \param[in] param parameter data
  */
void ParametricSvgItem::addParameter(const QString &pName, const Parameter &param)
{
    if(pName.isEmpty()){
        return;
    }
    if(!param.value.isValid() || param.value.isNull()){
        return;
    }
    m_parameters[pName] = param;
}

/*!
  * Convert XML QDomNode to Parameter
  *
  * \param[in] node XML node
  * \return Parameter. An empty Parameter on error
  */
ParametricSvgItem::Parameter ParametricSvgItem::domNodeToParameter(const QDomNode &node)
{
    Parameter result;
    bool ok;

    if(node.isNull() || !node.isElement()){
        return Parameter();
    }

    //Name
    QString nameParam = node.toElement().attribute("param");
    if(nameParam.isEmpty()){
        return Parameter();
    }
    result.name = nameParam;

    //Value
    QString valueParam = node.toElement().attribute("value");
    if(valueParam.isEmpty()){
        return Parameter();
    }
    double value = valueParam.toDouble(&ok);
    if(ok){
        result.value = QVariant(value);
    }else{
        result.value = QVariant(valueParam);
    }

    //Min limit
    QString minParam = node.toElement().attribute("min");

    double min = minParam.toDouble(&ok);
    if(ok){
        result.min = min;
    }else{
        result.min = -99999.0;
    }

    //Max limit
    QString maxParam = node.toElement().attribute("max");
    double max = maxParam.toDouble(&ok);
    if(ok){
        result.max = max;
    }else{
        result.max = 99999.0;
    }

    return result;
}

/*!
  * Read expressions from SVG and add it to expression list
  *
  * \param[in] node XML node with parameters declaration
  * \return true on succes
  */
bool ParametricSvgItem::readExpressions(const QDomNode &node)
{
    if(node.isNull()){
        return false;
    }

    QDomNodeList exps = node.toElement().elementsByTagName(getNamespace() + ":expression");
    if(!exps.isEmpty()){
        m_expressions.clear();
    }
    for(int i=0; i<exps.size(); ++i){
        Expression exp = domNodeToExpression(exps.at(i));
        addExpression(exp);
    }

    return true;
}

/*!
  * Convert XML QDomNode to Expression
  *
  * \param[in] node XML node
  * \return Expression. An empty Expression on error
  */
ParametricSvgItem::Expression ParametricSvgItem::domNodeToExpression(const QDomNode &node)
{
    if(node.isNull() || !node.isElement()){
        return Expression();
    }

    //Name
    QString variableName = node.toElement().attribute("var");
    if(variableName.isEmpty()){
        return Expression();
    }

    //Value
    QString variableValue = node.toElement().attribute("exp");
    if(variableValue.isEmpty()){
        //If the 'exp' attribute is empty,
        //then the value is read from the node text (CDATA is acceptable).
        variableValue = node.toElement().text();
        if(variableValue.isEmpty()){
            return Expression();
        }
    }
    Expression exp;
    exp.name = variableName;
    exp.value = variableValue;

    return exp;
}

/*!
  * Add Expression to list
  *
  * \param[in] exp Expresion
  */
void ParametricSvgItem::addExpression(const ParametricSvgItem::Expression &exp)
{
    if(exp.name.isEmpty())
        return;
    m_expressions.append(exp);
}

/*!
  * Evaluate parameters, expressions and string templates in SVG document
  */
void ParametricSvgItem::evaluateAll()
{
    QJSEngine jsEngine;
    evaluateParameters(&jsEngine);
    evaluateExpressions(&jsEngine);
    evaluateXmlDocument(&jsEngine);
}

/*!
  * Evaluate parameters and create equivalent variables in the JavaScript space
  *
  * \param[in] jsEngine link to JavaScript Engine
  */
void ParametricSvgItem::evaluateParameters(QJSEngine *jsEngine)
{
    QJSValue globalObj = jsEngine->globalObject();

    QMutableMapIterator<QString, Parameter> i(m_parameters);
    while (i.hasNext()) {
        i.next();                

        QString valueS;

        if (i.value().value.type() == QVariant::String){
            valueS = QString("`%1`").arg(i.value().value.toString());
        } else {
            valueS = i.value().value.toString();
        }

        QJSValue jsValue = jsEngine->evaluate(valueS);
        if(!jsValue.isError()){
            globalObj.setProperty(i.key(), jsValue);
        }
        addError(jsValue.isError(), jsValue.property("message").toString());
    }
}

/*!
  * Evaluate expression and create equivalent variables in the JavaScript space
  *
  * \param[in] jsEngine link to JavaScript Engine
  */
void ParametricSvgItem::evaluateExpressions(QJSEngine *jsEngine)
{
    QJSValue globalObj = jsEngine->globalObject();
    for (int i = 0; i < m_expressions.size(); ++i) {
        QJSValue jsValue = jsEngine->evaluate(m_expressions.at(i).value);

        if(!jsValue.isError()){
            globalObj.setProperty(m_expressions.at(i).name, jsValue);
        }
        addError(jsValue.isError(), jsValue.property("message").toString());
    }//for

    //Обновить значения параметров, если они вычислялись в выражениях
    expressionValuesToParameterValues(jsEngine);
}

void ParametricSvgItem::expressionValuesToParameterValues(QJSEngine *jsEngine)
{
    QJSValue globalObj = jsEngine->globalObject();

    QMutableMapIterator<QString, Parameter> i(m_parameters);
    while (i.hasNext()) {
        i.next();

        if(globalObj.hasProperty(i.key())){
            QJSValue value = globalObj.property(i.key());
            QVariant variantValue = value.toVariant();
            if(variantValue.isValid() && i.value().value != variantValue)
                setParameter(i.key(), variantValue);
        }
    }//while
}

/*!
  * Evaluate tamplates string in SVG document
  *
  * \param[in] jsEngine link to JavaScript Engine
  */
void ParametricSvgItem::evaluateXmlDocument(QJSEngine *jsEngine)
{
    traverseXmlNode(m_xmlDoc.documentElement(), jsEngine);
}

/*!
  * Traverse all SVG nodes and replace content with parameter or expression value
  *
  * \param[in] node link to parent XML node
  * \param[in] jsEngine link to JavaScript Engine
  */
void ParametricSvgItem::traverseXmlNode(const QDomNode &node, QJSEngine *jsEngine)
{
    QDomNode domNode = node.firstChild();

    while (!(domNode.isNull())) {
        if (domNodeIsValid(domNode)) {
            QDomNamedNodeMap attributesMap = domNode.attributes();
            for(int i=0; i<attributesMap.count(); ++i){
                QDomNode attribut = attributesMap.item(i);
                if(attribut.nodeName().startsWith(getNamespace())){
                    QString attName = getLocalName(attribut.nodeName());
                    QString attValue = attribut.nodeValue();
                    QJSValue jsResult = jsEngine->evaluate(attValue);

                    addError(jsResult.isError(), jsResult.property("message").toString());
                    if(jsResult.isError()){
                        continue;
                    }

                    QDomNode nodeForPatch;
                    if (attName.toLower() == "text") {
                        nodeForPatch = domNode.firstChild();
                    }else {
                        nodeForPatch = domNode.toElement().attributeNode(attName);
                    }
                    if (!nodeForPatch.isNull()) {
                       nodeForPatch.setNodeValue(jsResult.toString());
                    }
                }
            }
        }

        traverseXmlNode(domNode, jsEngine);
        domNode = domNode.nextSibling();
    }
}

/*!
  * Check that the node is not empty and has attributes
  *
  * \param[in] node link to XML node
  * \return true if valid
  */
bool ParametricSvgItem::domNodeIsValid(const QDomNode &node)
{
    return !node.isNull() && node.isElement() && node.hasAttributes();
}

QString ParametricSvgItem::getLocalName(const QString &qName)
{
    return getToken(qName, 1);
}

QString ParametricSvgItem::getUri(const QString &qName)
{
    return getToken(qName, 0);
}

QString ParametricSvgItem::getToken(const QString &qName, const int index, const QString delimeter)
{
    if(index < 0){
        return QString();
    }
    QStringList list = qName.split(delimeter);
    if(list.size() < 2){
        return QString();
    }

    return list[index];
}

/*!
  * Slot for changing parameter by qreal value
  *
  * \param[in] pName parameter name
  * \param[in] d value
  */
void ParametricSvgItem::changeParamByName(const QString &pName, qreal d)
{
    if(pName.isEmpty()){
        return;
    }
    updateByParameter(pName, QVariant(d));
}

int ParametricSvgItem::type() const
{
    return Type;
}

/*!
  * Set parameter value
  *
  * \param[in] pName paramter name
  * \param[in] value parameter value
  * \return true in success
  */
bool ParametricSvgItem::setParameter(const QString &pName, QVariant value)
{
    if(pName.isEmpty()){
        return false;
    }
    if(!value.isValid() || value.isNull()){
        return false;
    }
    if(m_parameters.contains(pName)){
        if (value.type() == QVariant::String) {
            m_parameters[pName].value = value;
            return true;
        }
        if(isNumberValueInRange(m_parameters.value(pName), value)){
            m_parameters[pName].value = value;
            return true;
        }    
    }

    return false;
}

bool ParametricSvgItem::isNumberValueInRange(const ParametricSvgItem::Parameter &param, const QVariant value)
{
    if(value.toReal() >= param.min
            && value.toReal() <= param.max){
        return true;
    }
    return false;
}

void ParametricSvgItem::addError(bool isError, QString message)
{
    if(isError)
        m_errors.append(message);
}

/*!
  * Set parameter value, evaluate and update graphics, using calculated values
  *
  * \param[in] pName parameter name
  * \param[in] value parameter value
  * \return true in success
  */
bool ParametricSvgItem::updateByParameter(const QString &pName, QVariant value)
{
    bool isParamWasSet = setParameter(pName, value);
    if(!isParamWasSet){
        return false;
    }

    evaluateAll();
    redraw();

    return true;
}

/*!
  * Return parameter value type
  *
  * \param[in] pName parameter name
  * \return QVariant::Type
  */
QVariant::Type ParametricSvgItem::parameterType(const QString &pName) const
{
    if(parameterIsExist(pName)){
        return m_parameters[pName].value.type();
    }
    return QVariant::Invalid;
}

/*!
  * Return parameter value
  *
  * \param[in] pName parameter name
  * \return QVariant value
  */
QVariant ParametricSvgItem::parameterValue(const QString &pName) const
{
    if(parameterIsExist(pName)){
        return m_parameters[pName].value;
    }
    return QVariant();
}

/*!
  * Return parameter value minimal limit
  *
  * \param[in] pName parameter name
  * \return minimal limit
  */
qreal ParametricSvgItem::parameterMin(const QString &pName) const
{
    if(parameterIsExist(pName)){
        return m_parameters[pName].min;
    }
    return 0.0;
}

/*!
  * Return parameter value maximal limit
  *
  * \param[in] pName parameter name
  * \return maximal limit
  */
qreal ParametricSvgItem::parameterMax(const QString &pName) const
{
    if(parameterIsExist(pName)){
        return m_parameters[pName].max;
    }
    return 0.0;
}

/*!
  * Check if the parameter exist in the item
  *
  * \param[in] pName parameter name
  * \return true, if the parameter exist in the item
  */
bool ParametricSvgItem::parameterIsExist(const QString &pName) const
{
    return m_parameters.contains(pName);
}

/*!
  * Get list of all parameter names
  *
  * \return list of names
  */
QStringList ParametricSvgItem::parameterNames() const
{
    return m_parameters.keys();
}

/*!
  * Get number of all parameters
  *
  * \return number of
  */
int ParametricSvgItem::parametersCount()
{
    return m_parameters.count();
}

bool ParametricSvgItem::isError()
{
    return m_errors.size() > 0;
}

QStringList &ParametricSvgItem::errors()
{
    return m_errors;
}





