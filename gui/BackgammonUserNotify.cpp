/*******************************************************************************
 * gui/BackgammonUserNotify.cpp                                                *
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

#include "BackgammonUserNotify.h"

#include "BackgammonConstants.h"
#include "services/BackgammonService.h"

#include "gui/MainWindow.h"

BackgammonUserNotify::BackgammonUserNotify(MainPage *page, QObject *parent)
    : UserNotify(parent),
      m_page(page)
{
    connect(&BackgammonService::instance(), &BackgammonService::gamesChanged,
            this, &UserNotify::updateIcon);
}

QIcon BackgammonUserNotify::getIcon()
{
    return QIcon(ICON_PATH);
}

QIcon BackgammonUserNotify::getMainIcon(bool)
{
    return QIcon(ICON_PATH);
}

unsigned int BackgammonUserNotify::getNewCount()
{
    return BackgammonService::instance().pendingInvitations();
}

void BackgammonUserNotify::iconClicked()
{
    MainWindow::showWindow(m_page);
}
