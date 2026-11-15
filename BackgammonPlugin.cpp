/*******************************************************************************
 * BackgammonPlugin.cpp                                                        *
 *                                                                             *
 * Copyright (C) 2026 RetroShare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "BackgammonPlugin.h"

#include "gui/BackgammonPage.h"
#include "gui/BackgammonSettingsPage.h"
#include "gui/toaster/BackgammonToasterNotify.h"
#include "gui/BoardTheme.h"
#include "gui/BackgammonSettings.h"
#include "services/BackgammonService.h"

#include "gui/settings/rsharesettings.h"

#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsversion.h>

#include <QIcon>
#include <QMessageBox>
#include <QObject>
#include <QSize>

#include <cstdlib>

#define IMAGE_Backgammon ":/images/backgammon.png"

extern "C" {
    // The function RetroShare calls to get the plugin. It must keep C linkage
    // and return an RsPlugin*.
    void *RETROSHARE_PLUGIN_provide()
    {
        static BackgammonPlugin *p = new BackgammonPlugin();
        return (void *)p;
    }

    // Revision of the RetroShare build; RetroShare loads plugins built for
    // the same revision automatically.
    uint32_t RETROSHARE_PLUGIN_revision = abs(atoi(RS_EXTRA_VERSION));

    // Plugin API version this plugin was built against.
    uint32_t RETROSHARE_PLUGIN_api = RS_PLUGIN_API_VERSION;
}

BackgammonPlugin::BackgammonPlugin()
{
}

BackgammonPlugin::~BackgammonPlugin()
{
    delete mToasterNotify;
    delete mIcon;
}

void BackgammonPlugin::getPluginVersion(int &major, int &minor, int &build, int &svn_rev) const
{
    major = RS_MAJOR_VERSION;
    minor = RS_MINOR_VERSION;
    build = RS_MINI_VERSION;
    svn_rev = abs(atoi(RS_EXTRA_VERSION));
}

void BackgammonPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
    mPlugInHandler = pgHandler;
}

void BackgammonPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
    BackgammonService::instance().connectToGxsTunnelService(interfaces.mGxsTunnels);
}

void BackgammonPlugin::stop()
{
    // GxsTunnel keeps calling the (never deleted) service after the plugin
    // is deleted: make it ignore everything from now on.
    BackgammonService::instance().shutdown();
}

MainPage *BackgammonPlugin::qt_page() const
{
    if (!mMainPage) {
        BackgammonService::instance().setContactsOnly(
            Settings->valueFromGroup("Backgammon", "ContactsOnly", false).toBool());
        BoardTheme::setCurrent(
            Settings->valueFromGroup("Backgammon", "BoardTheme", QString()).toString());
        BackgammonSettings::applyDebugLogging();
        mMainPage = new BackgammonPage();
        mMainPage->setPageName(QObject::tr("Backgammon"));
        if (QIcon *icon = qt_icon())
            mMainPage->setIconPixmap(*icon);
    }
    return mMainPage;
}

QIcon *BackgammonPlugin::qt_icon() const
{
    if (!mIcon) {
        Q_INIT_RESOURCE(Backgammon_images);
        mIcon = new QIcon(IMAGE_Backgammon);
        // Drawn for small sizes, so the board stays readable in the toolbar.
        mIcon->addFile(QStringLiteral(":/images/backgammon-32.png"), QSize(32, 32));
    }
    return mIcon;
}

ToasterNotify *BackgammonPlugin::qt_toasterNotify()
{
    if (!mToasterNotify)
        mToasterNotify = new BackgammonToasterNotify();
    return mToasterNotify;
}

ConfigPage *BackgammonPlugin::qt_config_page() const
{
    // Settings pages are deleted when the settings window closes: never cache it.
    return new BackgammonSettingsPage();
}

QDialog *BackgammonPlugin::qt_about_page() const
{
    static QMessageBox *about = nullptr;
    if (!about) {
        about = new QMessageBox();
        about->setWindowTitle(QObject::tr("Backgammon"));
        about->setText(QObject::tr(
            "<h3>RetroShare Backgammon plugin</h3>"
            "<p>Play Backgammon and Long Nardy with your contacts over encrypted GXS tunnels.</p>"));
        about->setStandardButtons(QMessageBox::Ok);
    }
    return about;
}

QTranslator *BackgammonPlugin::qt_translator(QApplication *, const QString &, const QString &) const
{
    return nullptr;
}

std::string BackgammonPlugin::getShortPluginDescription() const
{
    return "Backgammon, play Backgammon and Long Nardy with your contacts.";
}

std::string BackgammonPlugin::getPluginName() const
{
    return "Backgammon";
}
