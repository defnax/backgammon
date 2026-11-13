/*******************************************************************************
 * gui/BackgammonUserNotify.h                                                  *
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

#include "gui/common/UserNotify.h"

class MainPage;

// The counter on the Backgammon toolbar button: invitations not answered yet.
class BackgammonUserNotify : public UserNotify
{
    Q_OBJECT

public:
    BackgammonUserNotify(MainPage *page, QObject *parent);

private:
    QIcon getIcon() override;
    QIcon getMainIcon(bool hasNew) override;
    unsigned int getNewCount() override;
    void iconClicked() override;

    MainPage *m_page;
};
