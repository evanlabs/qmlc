/*!
 * Copyright (C) 2014 Nomovok Ltd. All rights reserved.
 * Contact: info@nomovok.com
 *
 * This file may be used under the terms of the GNU Lesser
 * General Public License version 2.1 as published by the Free Software
 * Foundation and appearing in the file LICENSE.LGPL included in the
 * packaging of this file.  Please review the following information to
 * ensure the GNU Lesser General Public License version 2.1 requirements
 * will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 *
 * In addition, as a special exception, copyright holders
 * give you certain additional rights.  These rights are described in
 * the Digia Qt LGPL Exception version 1.1, included in the file
 * LGPL_EXCEPTION.txt in this package.
 */

#include <QQmlFile>
#include <private/qobject_p.h>
#include <private/qqmlengine_p.h>

#include "compiler.h"
#include "qmlcompilation.h"
#include "qmcexporter.h"

class CompilerPrivate : QObjectPrivate
{
    Q_DECLARE_PUBLIC(Compiler)

public:
    CompilerPrivate();
    virtual ~CompilerPrivate();

    QList<QQmlError> errors;
    QmlCompilation* compilation;
};

CompilerPrivate::CompilerPrivate()
    : compilation(NULL)
{
}

CompilerPrivate::~CompilerPrivate()
{
    delete compilation;
}

Compiler::Compiler(QObject *parent) :
    QObject(*(new CompilerPrivate), parent)
{
}

Compiler::~Compiler()
{
}

QmlCompilation* Compiler::takeCompilation()
{
    Q_D(Compiler);
    QmlCompilation* c = d->compilation;
    d->compilation = NULL;
    return c;
}

bool Compiler::loadData()
{
    Q_D(Compiler);
    const QUrl& url = d->compilation->url;
    if (!url.isValid() || url.isEmpty())
        return false;
    QQmlFile f;
    f.load(d->compilation->engine, url);
    if (!f.isReady()) {
        if (f.isError()) {
            QQmlError error;
            error.setUrl(url);
            error.setDescription(f.error());
            appendError(error);
        }
        return false;
    }
    d->compilation->code = QString::fromUtf8(f.data());
    return true;
}

void Compiler::clearError()
{
    Q_D(Compiler);
    d->errors.clear();
}

QList<QQmlError> Compiler::errors() const
{
    const Q_D(Compiler);
    return d->errors;
}

void Compiler::appendError(const QQmlError &error)
{
    Q_D(Compiler);
    d->errors.append(error);
}

void Compiler::appendErrors(const QList<QQmlError> &errors)
{
    Q_D(Compiler);
    d->errors.append(errors);
}

QmlCompilation* Compiler::compilation()
{
    Q_D(Compiler);
    return d->compilation;
}

const QmlCompilation* Compiler::compilation() const
{
    const Q_D(Compiler);
    return d->compilation;
}

bool Compiler::compile(const QString &url)
{
    Q_D(Compiler);
    clearError();

    // check that engine is using correct factory
    if (!qgetenv("QV4_FORCE_INTERPRETER").isEmpty()) {
        QQmlError error;
        error.setDescription("Compiler is forced to use interpreter");
        appendError(error);
        return false;
    }

    Q_ASSERT(d->compilation == NULL);
    QmlCompilation* c = new QmlCompilation(url, QUrl(url));
    d->compilation = c;
    c->importCache = new QQmlImports(&QQmlEnginePrivate::get(d->compilation->engine)->typeLoader);
    c->importDatabase = new QQmlImportDatabase(d->compilation->engine);
    c->url = url;

    if (!loadData()) {
        delete takeCompilation();
        return false;
    }

    if (!compileData()) {
        delete takeCompilation();
        return false;
    }

    return true;
}

bool Compiler::compile(const QString &url, QDataStream &output)
{
    bool ret = compile(url);
    if (ret) {
        ret = createExportStructures();
        if (ret)
            ret = exportData(output);
    }

    delete takeCompilation();

    return ret;
}

bool Compiler::exportData(QDataStream &output)
{
    Q_D(Compiler);

    if (!d->compilation->checkData()) {
        QQmlError error;
        error.setDescription("Compiled data not valid. Internal error.");
        appendError(error);
        return false;
    }

    QmcExporter exporter(d->compilation);
    bool ret = exporter.exportQmc(output);
    if (!ret) {
        QQmlError error;
        error.setDescription("Error saving data");
        appendError(error);
    }
    return ret;
}

QString Compiler::stringAt(int index) const
{
    const Q_D(Compiler);
    return d->compilation->document->jsGenerator.stringTable.stringForIndex(index);
}

bool Compiler::addImport(const QV4::CompiledData::Import *import, QList<QQmlError> *errors)
{
    Q_D(Compiler);
    const QString &importUri = stringAt(import->uriIndex);
    const QString &importQualifier = stringAt(import->qualifierIndex);

    if (import->type == QV4::CompiledData::Import::ImportScript) {
        qDebug() << "Script imported" << importUri;
        // TBD: qqmltypeloader.cpp:1320
        QmlCompilation::ScriptReference scriptRef;
        scriptRef.location = import->location;
        scriptRef.qualifier = importQualifier;
        d->compilation->scripts.append(scriptRef);
    } else if (import->type == QV4::CompiledData::Import::ImportLibrary) {
        // TBD: locked module qqmltypeloader.cpp:1325
        // TBD: qmldir qqmltypeloader.cpp:1331
        // assume it is module
        if (QQmlMetaType::isAnyModule(importUri)) {
            if (!d->compilation->importCache->addLibraryImport(d->compilation->importDatabase, importUri, importQualifier, import->majorVersion,
                                          import->minorVersion, QString(), QString(), false, errors))
                return false;
        } // TBD: else add to unresolved imports qqmltypeloader.cpp:1356
    } else {
        Q_ASSERT(import->type == QV4::CompiledData::Import::ImportFile);
        qDebug() << "File import type not supported";
        // TBD: qqmltypeloader.cpp:1383
        return false;
    }
    return true;
}
