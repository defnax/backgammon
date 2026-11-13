/*******************************************************************************
 * gui/BackgammonPage.cpp                                                      *
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

#include "BackgammonPage.h"
#include "ui_BackgammonPage.h"

#include "BackgammonGameWindow.h"
#include "BackgammonUserNotify.h"
#include "ui_InvitationItem.h"
#include "services/BackgammonService.h"

#include "gui/common/FriendSelectionWidget.h"
#include "gui/gxs/GxsIdChooser.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"

#include <QMenu>
#include <QPointer>
#include <QMessageBox>

namespace {
const QString SettingsGroup = QStringLiteral("Backgammon");
constexpr int IdRole = Qt::UserRole + 1;   // player's identity in "My players"
}

BackgammonPage::BackgammonPage(QWidget *parent)
    : MainPage(parent),
      ui(new Ui::BackgammonPage)
{
    ui->setupUi(this);

    // The "New game" column keeps its minimum width; "Your games" takes the rest.
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter->setSizes({ ui->newGameGroup->minimumWidth(), width() });

    BackgammonService &service = BackgammonService::instance();

    const RsGxsId lastId(Settings->valueFromGroup(SettingsGroup, "Identity", QString())
                             .toString().toStdString());
    ui->ownIdChooser->loadIds(IDCHOOSER_ID_REQUIRED, lastId);

    ui->opponentSelection->setModus(FriendSelectionWidget::MODUS_SINGLE);
    ui->opponentSelection->setShowType(FriendSelectionWidget::SHOW_GXS);
    ui->opponentSelection->start();
    // Right-click an identity: the menu of the list gets "Invite to play".
    ui->opponentSelection->addContextMenuAction(ui->actionInvite);
    ui->opponentSelection->addContextMenuAction(ui->actionAddPlayer);

    loadPlayers();
    // Start on "My players" once there are some.
    ui->opponentTabs->setCurrentWidget(ui->playersList->topLevelItemCount() ? ui->playersTab : ui->identitiesTab);

    ui->variantCombo->setCurrentIndex(
        Settings->valueFromGroup(SettingsGroup, "Variant", 0).toInt() == 1 ? 1 : 0);
    ui->lengthSpin->setRange(MIN_NUM_GAMES, MAX_NUM_GAMES);
    ui->lengthSpin->setValue(Settings->valueFromGroup(SettingsGroup, "Length", 1).toInt());

    connect(ui->inviteButton, &QPushButton::clicked, this, &BackgammonPage::invite);
    connect(ui->actionInvite, &QAction::triggered, this, &BackgammonPage::invite);
    connect(ui->actionAddPlayer, &QAction::triggered, this, &BackgammonPage::addSelectedIdentity);
    connect(ui->actionRemovePlayer, &QAction::triggered, this, &BackgammonPage::removeSelectedPlayer);
    connect(ui->playersList, &QTreeWidget::customContextMenuRequested, this, &BackgammonPage::playersMenu);
    connect(ui->playersList, &QTreeWidget::itemDoubleClicked, this, &BackgammonPage::invite);
    connect(&service, &BackgammonService::playerMet, this, &BackgammonPage::addPlayer);
    connect(ui->gamesList, &QListWidget::itemDoubleClicked, this, &BackgammonPage::showGame);
    connect(ui->gamesList, &QListWidget::customContextMenuRequested, this, &BackgammonPage::gamesMenu);
    connect(ui->actionAcceptInvite, &QAction::triggered, this, &BackgammonPage::acceptSelectedInvitation);
    connect(ui->actionDeclineInvite, &QAction::triggered, this, &BackgammonPage::declineSelectedInvitation);
    connect(ui->actionShowGame, &QAction::triggered, this, [this] { showGame(ui->gamesList->currentItem()); });
    connect(&service, &BackgammonService::gamesChanged, this, &BackgammonPage::refreshGames);

    refreshGames();
    updateInviteButton();
}

BackgammonPage::~BackgammonPage()
{
    delete ui;
}

void BackgammonPage::updateInviteButton()
{
    const bool connected = BackgammonService::instance().isConnected();
    ui->serviceStatusLabel->setVisible(!connected);

    // Everything is checked when the button is clicked (opponent chosen,
    // tunnel service ready), with a message saying what is missing.
    ui->inviteButton->setEnabled(true);
}

void BackgammonPage::invite()
{
    RsGxsId own;
    if (ui->ownIdChooser->getChosenId(own) != GxsIdChooser::KnowId || own.isNull()) {
        QMessageBox::warning(this, tr("Backgammon"), tr("Please choose the identity you want to play with."));
        return;
    }

    const RsGxsId peer = selectedOpponent();
    if (peer.isNull()) {
        QMessageBox::warning(this, tr("Backgammon"), tr("Please choose an opponent."));
        return;
    }
    if (peer == own) {
        QMessageBox::warning(this, tr("Backgammon"), tr("You cannot play against yourself."));
        return;
    }

    const bg::Variant gameType = ui->variantCombo->currentIndex() == 1
                                     ? bg::Variant::LongNardy : bg::Variant::Backgammon;
    const int length = ui->lengthSpin->value();

    Settings->setValueToGroup(SettingsGroup, "Identity", QString::fromStdString(own.toStdString()));
    Settings->setValueToGroup(SettingsGroup, "Variant", ui->variantCombo->currentIndex());
    Settings->setValueToGroup(SettingsGroup, "Length", length);

    QString error;
    if (!BackgammonService::instance().invite(peer, own, gameType, length, &error))
        QMessageBox::warning(this, tr("Backgammon"), error);
}

RsGxsId BackgammonPage::selectedOpponent() const
{
    if (ui->opponentTabs->currentWidget() == ui->playersTab) {
        const QTreeWidgetItem *item = ui->playersList->currentItem();
        return item ? RsGxsId(item->data(0, IdRole).toString().toStdString()) : RsGxsId();
    }
    FriendSelectionWidget::IdType type = FriendSelectionWidget::IDTYPE_NONE;
    const std::string id = ui->opponentSelection->selectedId(type);
    return type == FriendSelectionWidget::IDTYPE_GXS ? RsGxsId(id) : RsGxsId();
}

void BackgammonPage::loadPlayers()
{
    const QStringList ids = Settings->valueFromGroup(SettingsGroup, "Players", QStringList()).toStringList();
    for (const QString &id : ids)
        addPlayer(RsGxsId(id.toStdString()));
}

void BackgammonPage::savePlayers() const
{
    QStringList ids;
    for (int i = 0; i < ui->playersList->topLevelItemCount(); ++i)
        ids << ui->playersList->topLevelItem(i)->data(0, IdRole).toString();
    Settings->setValueToGroup(SettingsGroup, "Players", ids);
}

void BackgammonPage::addPlayer(const RsGxsId &id)
{
    if (id.isNull())
        return;
    const QString key = QString::fromStdString(id.toStdString());
    for (int i = 0; i < ui->playersList->topLevelItemCount(); ++i)
        if (ui->playersList->topLevelItem(i)->data(0, IdRole).toString() == key)
            return;

    // Shows the name and avatar of the identity once they are known.
    auto *item = new GxsIdRSTreeWidgetItem(nullptr, GxsIdDetails::ICON_TYPE_AVATAR, true, ui->playersList);
    item->setId(id, 0, true);
    item->setData(0, IdRole, key);
    savePlayers();
}

void BackgammonPage::addSelectedIdentity()
{
    FriendSelectionWidget::IdType type = FriendSelectionWidget::IDTYPE_NONE;
    const std::string id = ui->opponentSelection->selectedId(type);
    if (type == FriendSelectionWidget::IDTYPE_GXS)
        addPlayer(RsGxsId(id));
}

void BackgammonPage::removeSelectedPlayer()
{
    delete ui->playersList->currentItem();
    savePlayers();
}

void BackgammonPage::playersMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = ui->playersList->itemAt(pos);
    if (!item)
        return;
    ui->playersList->setCurrentItem(item);
    QMenu menu(this);
    menu.addAction(ui->actionInvite);
    menu.addSeparator();
    menu.addAction(ui->actionRemovePlayer);
    menu.exec(ui->playersList->viewport()->mapToGlobal(pos));
}

UserNotify *BackgammonPage::createUserNotify(QObject *parent)
{
    return new BackgammonUserNotify(this, parent);
}

BackgammonGameWindow *BackgammonPage::selectedGame() const
{
    const QListWidgetItem *item = ui->gamesList->currentItem();
    QObject *object = item ? item->data(Qt::UserRole).value<QObject *>() : nullptr;
    // The list only holds live windows, but check anyway.
    for (BackgammonGameWindow *window : BackgammonService::instance().games())
        if (window == object)
            return window;
    return nullptr;
}

void BackgammonPage::acceptSelectedInvitation()
{
    if (BackgammonGameWindow *window = selectedGame())
        window->acceptInvitation();
}

void BackgammonPage::declineSelectedInvitation()
{
    if (BackgammonGameWindow *window = selectedGame())
        window->declineInvitation();
}

void BackgammonPage::gamesMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->gamesList->itemAt(pos);
    if (!item || !(item->flags() & Qt::ItemIsEnabled))
        return;
    ui->gamesList->setCurrentItem(item);
    const BackgammonGameWindow *window = selectedGame();
    if (!window)
        return;
    QMenu menu(this);
    if (window->phase() == BackgammonGameWindow::Offered) {
        menu.addAction(ui->actionAcceptInvite);
        menu.addAction(ui->actionDeclineInvite);
        menu.addSeparator();
    }
    menu.addAction(ui->actionShowGame);
    menu.exec(ui->gamesList->viewport()->mapToGlobal(pos));
}

void BackgammonPage::refreshGames()
{
    // Keep the selection across the refresh.
    const QObject *selected = selectedGame();
    ui->gamesList->clear();
    for (BackgammonGameWindow *window : BackgammonService::instance().games()) {
        const bool invitation = window->phase() == BackgammonGameWindow::Offered;
        const QString game = window->variant() == bg::Variant::LongNardy ? tr("Long Nardy") : tr("Backgammon");
        auto *item = new QListWidgetItem(ui->gamesList);
        item->setData(Qt::UserRole, QVariant::fromValue<QObject *>(window));
        if (invitation) {
            // An invitation row carries its own Accept and Decline buttons.
            auto *row = new QWidget;
            Ui::InvitationItem rowUi;
            rowUi.setupUi(row);
            // "Invitation from Bob:" [Accept] [Decline] "Backgammon to 3 points"
            const QString details = tr("%1 to %2").arg(game, tr("%n point(s)", "", window->matchLength()));
            const QString text = tr("Invitation from %1: %2").arg(window->opponentName(), details);
            rowUi.textLabel->setText(tr("Invitation from %1:").arg(window->opponentName()));
            rowUi.detailsLabel->setText(details);
            QPointer<BackgammonGameWindow> guard(window);
            connect(rowUi.acceptButton, &QPushButton::clicked, this, [guard] {
                if (guard)
                    guard->acceptInvitation();
            });
            connect(rowUi.declineButton, &QPushButton::clicked, this, [guard] {
                if (guard)
                    guard->declineInvitation();
            });
            item->setToolTip(text);
            item->setSizeHint(row->sizeHint());
            ui->gamesList->setItemWidget(item, row);
        } else {
            item->setText(window->summary());
            item->setIcon(QIcon(ICON_PATH));
        }
        if (window == selected)
            ui->gamesList->setCurrentItem(item);
    }
    if (ui->gamesList->count() == 0) {
        auto *item = new QListWidgetItem(tr("No games yet"), ui->gamesList);
        item->setFlags(Qt::NoItemFlags);
    }
    updateInviteButton();
}

void BackgammonPage::showGame(QListWidgetItem *item)
{
    QObject *object = item ? item->data(Qt::UserRole).value<QObject *>() : nullptr;
    // The list only holds live windows: it is rebuilt whenever one closes.
    for (BackgammonGameWindow *window : BackgammonService::instance().games()) {
        if (window == object) {
            window->showNormal();
            window->raise();
            window->activateWindow();
            return;
        }
    }
}

