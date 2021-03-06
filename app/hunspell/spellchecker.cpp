/*
 * Copyright 2013 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "spellchecker.h"
using hunspell::SpellChecker;

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QTextCodec>

#include "hunspell/hunspell.hxx"

#include <spellchecker/dictionary.h>
#include <datalocation.h>

SpellChecker::SpellChecker() :
    hunspellChecker {nullptr},
    textCodec {nullptr}
{
}

SpellChecker::~SpellChecker()
{
    delete hunspellChecker;
}

bool SpellChecker::isCorrect(const QString &word)
{
    if (!textCodec || !hunspellChecker) {
        return true;
    }

    return hunspellChecker->spell(word.toStdString()) != 0;
}

QStringList SpellChecker::suggestions(const QString &word)
{
    QStringList suggestions;

    if (!textCodec || !hunspellChecker) {
        return suggestions;
    }

    const std::vector<std::string> suggestionsList = hunspellChecker->suggest(word.toStdString());
    suggestions.reserve(suggestionsList.size());

    for (const auto &sugg : suggestionsList) {
        suggestions << QString::fromStdString(sugg);
    }

    return suggestions;
}

void SpellChecker::addToUserWordlist(const QString &word)
{
    hunspellChecker->add(textCodec->fromUnicode(word).constData());
    if(!userWordlist.isEmpty()) {
        QFile userWordlistFile(userWordlist);
        if(!userWordlistFile.open(QIODevice::Append))
            return;

        QTextStream stream(&userWordlistFile);
        stream << word << "\n";
        userWordlistFile.close();
    }
}

void SpellChecker::loadDictionary(const QString &dictFilePath)
{
    delete hunspellChecker;

    qDebug() << "Load dictionary from path" << dictFilePath;

    QString affixFilePath(dictFilePath);
    affixFilePath.replace(QLatin1String(".dic"), QLatin1String(".aff"));

    hunspellChecker = new Hunspell(affixFilePath.toLocal8Bit(), dictFilePath.toLocal8Bit());

    textCodec = QTextCodec::codecForName(hunspellChecker->get_dic_encoding());
    if (!textCodec) {
        textCodec = QTextCodec::codecForName("UTF-8");
    }

    // also load user word list
    QString path = DataLocation::writableLocation();
    loadUserWordlist(path + "/user.dic");
}

void SpellChecker::loadUserWordlist(const QString &userWordlistPath)
{
    userWordlist = userWordlistPath;

    QFile userWordlistFile(userWordlistPath);
    if (!userWordlistFile.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&userWordlistFile);
    for (QString word = stream.readLine(); !word.isEmpty(); word = stream.readLine()) {
        hunspellChecker->add(textCodec->fromUnicode(word).constData());
    }
}
