/*
 * Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <iostream>

#include <QDir>
#include <QDBusInterface>

#include <KApplication>
#include <KAboutData>
#include <KAction>
#include <KCmdLineArgs>
#include <KLocale>
#include <KService>
#include <KServiceTypeTrader>
#include <KShell>
#include <KStandardDirs>
#include <KProcess>
#include <KSycoca>
#include <KConfigGroup>

#include <Plasma/PackageStructure>
#include <Plasma/Package>
#include <Plasma/PackageMetadata>

static const char description[] = I18N_NOOP("Install, list, remove Plasma packages");
static const char version[] = "0.1";

void output(const QString &msg)
{
    std::cout << msg.toLocal8Bit().constData() << std::endl;
}

void runKbuildsycoca()
{
    QDBusInterface dbus("org.kde.kded", "/kbuildsycoca", "org.kde.kbuildsycoca");
    dbus.call(QDBus::Block, "recreate");
}

QStringList packages(const QString& type)
{
    QStringList result;
    KService::List services = KServiceTypeTrader::self()->query("Plasma/" + type);
    foreach(const KService::Ptr &service, services) {
        result << service->property("X-KDE-PluginInfo-Name", QVariant::String).toString();
    }
    return result;
}

void listPackages(const QString& type)
{
    QStringList list = packages(type);
    list.sort();
    foreach(const QString& package, list) {
        output(package);
    }
}

int main(int argc, char **argv)
{
    KAboutData aboutData("plasmapkg", 0, ki18n("Plasma Package Manager"),
                         version, ki18n(description), KAboutData::License_GPL,
                         ki18n("(C) 2008, Aaron Seigo"));
    aboutData.addAuthor( ki18n("Aaron Seigo"),
                         ki18n("Original author"),
                        "aseigo@kde.org" );

    KComponentData componentData(aboutData);

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("g");
    options.add("global", ki18n("For install or remove, operates on packages installed for all users."));
    options.add("t");
    options.add("type <type>",
                ki18nc("theme, wallpaper, etc. are keywords, but they may be translated, as both versions "
                       "are recognized by the application "
                       "(if translated, should be same as messages with 'package type' context below)",
                       "The type of package, e.g. theme, wallpaper, plasmoid, dataengine, runner, etc."),
                "plasmoid");
    options.add("s");
    options.add("i");
    options.add("install <path>", ki18nc("Do not translate <path>", "Install the package at <path>"));
    options.add("u");
    options.add("upgrade <path>", ki18nc("Do not translate <path>", "Upgrade the package at <path>"));
    options.add("l");
    options.add("list", ki18n("List installed packages"));
    options.add("r");
    options.add("remove <name>", ki18nc("Do not translate <name>", "Remove the package named <name>"));
    options.add("p");
    options.add("packageroot <path>", ki18n("Absolute path to the package root. If not supplied, then the standard data directories for this KDE session will be searched instead."));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    const QString type = args->getOption("type").toLower();
    QString packageRoot = type;
    QString servicePrefix;
    QString pluginType;
    Plasma::PackageStructure *installer = 0;

    if (type == i18nc("package type", "plasmoid") || type == "plasmoid") {
        packageRoot = "plasma/plasmoids/";
        servicePrefix = "plasma-applet-";
        pluginType = "Applet";
    } else if (type == i18nc("package type", "theme") || type == "theme") {
        packageRoot = "desktoptheme/";
    } else if (type == i18nc("package type", "wallpaper") || type == "wallpaper") {
        packageRoot = "wallpapers/";
    } else if (type == i18nc("package type", "dataengine") || type == "dataengine") {
        packageRoot = "plasma/dataengines/";
        servicePrefix = "plasma-dataengine-";
        pluginType = "DataEngine";
    } else if (type == i18nc("package type", "runner") || type == "runner") {
        packageRoot = "plasma/runners/";
        servicePrefix = "plasma-runner-";
        pluginType = "Runner";
    } else {
        QString constraint = QString("'%1' == [X-KDE-PluginInfo-Name]").arg(packageRoot);
        KService::List offers = KServiceTypeTrader::self()->query("Plasma/PackageStructure", constraint);
        if (offers.isEmpty()) {
            output(i18n("Could not find a suitable installer for package of type %1", type));
            return 1;
        }

        KService::Ptr offer = offers.first();
        QString error;
        installer = offer->createInstance<Plasma::PackageStructure>(0, QVariantList(), &error);

        if (!installer) {
            output(i18n("Could not load installer for package of type %1. Error reported was: %2",
                        type, error));
            return 1;
        }
        packageRoot = installer->defaultPackageRoot();
        pluginType = installer->type();
    }

    if (args->isSet("list")) {
        listPackages(pluginType);
    } else {
        // install, remove or upgrade
        if (!installer) {
            installer = new Plasma::PackageStructure();
            installer->setServicePrefix(servicePrefix);
        }

        if (args->isSet("packageroot")) {
            packageRoot = args->getOption("packageroot");
        } else if (args->isSet("global")) {
            packageRoot = KStandardDirs::locate("data", packageRoot);
        } else {
            packageRoot = KStandardDirs::locateLocal("data", packageRoot);
        }

        QString package;
        QString packageFile;
        if (args->isSet("remove")) {
            package = args->getOption("remove");
        } else if (args->isSet("upgrade")) {
            package = args->getOption("upgrade");
        } else if (args->isSet("install")) {
            package = args->getOption("install");
        }
        if (!QDir::isAbsolutePath(package)) {
            packageFile = QDir(QDir::currentPath() + '/' + package).absolutePath();
        } else {
            packageFile = package;
        }

        if (args->isSet("remove") || args->isSet("upgrade")) {
            installer->setPath(packageFile);
            Plasma::PackageMetadata metadata = installer->metadata();

            QString pluginName;
            if (metadata.pluginName().isEmpty()) {
                // plugin name given in command line
                pluginName = package;
            } else {
                // Parameter was a plasma package, get plugin name from the package
                pluginName = metadata.pluginName();
            }

            QStringList installed = packages(pluginType);
            if (installed.contains(pluginName)) {
                if (installer->uninstallPackage(pluginName, packageRoot)) {
                    output(i18n("Successfully removed %1", pluginName));
                } else if (!args->isSet("upgrade")) {
                    output(i18n("Removal of %1 failed.", pluginName));
		    delete installer;
                    return 1;
                }
            } else {
                output(i18n("Plugin %1 is not installed.", pluginName));
            }
        }
        if (args->isSet("install") || args->isSet("upgrade")) {
            if (installer->installPackage(packageFile, packageRoot)) {
                output(i18n("Successfully installed %1", packageFile));
            } else {
                output(i18n("Installation of %1 failed.", packageFile));
		delete installer;
                return 1;
            }
        }
        if (package.isEmpty()) {
            KCmdLineArgs::usageError(i18nc("No option was given, this is the error message telling the user he needs at least one, do not translate install, remove, upgrade nor list", "One of install, remove, upgrade or list is required."));
        } else {
            runKbuildsycoca();
        }
    }
    delete installer;
    return 0;
}

