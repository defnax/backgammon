/*******************************************************************************
 * gui/BackgammonPage.h                                                        *
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

#include <retroshare-gui/mainpage.h>
#include <retroshare/rsids.h>

namespace Ui { class BackgammonPage; }

class QListWidgetItem;

// The "Backgammon" page in RetroShare's main window: start a match against
// an identity and see the matches in progress.
class BackgammonPage : public MainPage
{
    Q_OBJECT

public:
    explicit BackgammonPage(QWidget *parent = nullptr);
    ~BackgammonPage() override;

    // Counter on the toolbar button: unanswered invitations.
    UserNotify *createUserNotify(QObject *parent) override;

private slots:
    void invite();
    void refreshGames();
    void updateInviteButton();
    void showGame(QListWidgetItem *item);

    // "My players": identities kept to invite them again quickly.
    void addPlayer(const RsGxsId &id);
    void addSelectedIdentity();
    void removeSelectedPlayer();
    void playersMenu(const QPoint &pos);

    void gamesMenu(const QPoint &pos);
    void acceptSelectedInvitation();
    void declineSelectedInvitation();

private:
    RsGxsId selectedOpponent() const;
    class BackgammonGameWindow *selectedGame() const;
    void loadPlayers();
    void savePlayers() const;

    Ui::BackgammonPage *ui;
};
