/*******************************************************************************
 * gui/BackgammonSettingsPage.cpp                                              *
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

#include "BackgammonSettingsPage.h"
#include "ui_BackgammonSettingsPage.h"

#include "BackgammonConstants.h"
#include "BackgammonSettingsWidget.h"

BackgammonSettingsPage::BackgammonSettingsPage(QWidget *parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags),
      ui(new Ui::BackgammonSettingsPage)
{
    ui->setupUi(this);
}

BackgammonSettingsPage::~BackgammonSettingsPage()
{
    delete ui;
}

void BackgammonSettingsPage::load()
{
    ui->settingsWidget->load();
}

QPixmap BackgammonSettingsPage::iconPixmap() const
{
    return QPixmap(ICON_PATH);
}

QString BackgammonSettingsPage::pageName() const
{
    return tr("Backgammon");
}

QString BackgammonSettingsPage::helpText() const
{
    return tr("Invitations, board colours, game sounds and debug logs of the Backgammon plugin. "
              "The same settings open from the settings button on the Backgammon page and in the game window.");
}
