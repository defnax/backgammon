/*******************************************************************************
 * BackgammonPlugin.h                                                          *
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

#pragma once

#include <retroshare/rsplugin.h>

class QIcon;

// Entry point of the plugin. It has no p3Service of its own: all traffic
// goes through GXS tunnels (see services/BackgammonService.h).
class BackgammonPlugin : public RsPlugin
{
public:
    BackgammonPlugin();
    ~BackgammonPlugin() override;

    MainPage *qt_page() const override;
    QIcon *qt_icon() const override;
    QDialog *qt_about_page() const override;
    ConfigPage *qt_config_page() const override;
    ToasterNotify *qt_toasterNotify() override;
    QTranslator *qt_translator(QApplication *app, const QString &languageCode,
                               const QString &externalDir) const override;

    void getPluginVersion(int &major, int &minor, int &build, int &svn_rev) const override;
    void setPlugInHandler(RsPluginHandler *pgHandler) override;
    void setInterfaces(RsPlugInInterfaces &interfaces) override;
    void stop() override;

    std::string getShortPluginDescription() const override;
    std::string getPluginName() const override;

private:
    mutable MainPage *mMainPage = nullptr;
    mutable QIcon *mIcon = nullptr;
    ToasterNotify *mToasterNotify = nullptr;
    RsPluginHandler *mPlugInHandler = nullptr;
};
