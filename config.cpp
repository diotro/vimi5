// Copyright 2009 cwk

#include "config.h"
#include <QDesktopServices>
#include <QSettings>

QString Config::collectionPath;
QStringList Config::movieSuffixes;

void Config::load()
{
    QSettings settings;

    collectionPath = settings.value("collectionPath", QDesktopServices::storageLocation(QDesktopServices::MoviesLocation)).toString();

    QStringList defaultSuffixes;
    defaultSuffixes << "*.mpg"
                    << "*.wmv"
                    << "*.mkv"
                    << "*.avi"
                    << "*.mp4";
    QStringList settingSuffixes = settings.value("movieSuffixes", defaultSuffixes).toStringList();
    if (settingSuffixes.length() > 0)
        movieSuffixes = settingSuffixes;
    else
        movieSuffixes = defaultSuffixes;
}

void Config::save()
{
    QSettings settings;
    settings.setValue("collectionPath", collectionPath);
    settings.setValue("movieSuffixes", movieSuffixes);
}
