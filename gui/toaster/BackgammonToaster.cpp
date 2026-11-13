/*******************************************************************************
 * gui/toaster/BackgammonToaster.cpp                                           *
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

#include "BackgammonToaster.h"

#include "gui/BackgammonGameWindow.h"
#include "gui/BackgammonConstants.h"

#include "gui/common/AvatarWidget.h"

BackgammonToaster::BackgammonToaster(BackgammonGameWindow *invitation)
    : QWidget(nullptr),
      m_invitation(invitation)
{
    ui.setupUi(this);
    ui.avatarWidget->setFrameType(AvatarWidget::NO_FRAME);
    ui.avatarWidget->setDefaultAvatar(ICON_PATH);
    ui.avatarWidget->setStyleSheet(
        QStringLiteral("AvatarWidget { border: none; padding: 0; background: transparent; }"));
    ui.toasterLabel->setText(tr("Backgammon invitation"));

    if (invitation) {
        ui.avatarWidget->setGxsId(invitation->peerId());
        const QString game = invitation->variant() == bg::Variant::LongNardy ? tr("Long Nardy") : tr("Backgammon");
        ui.textLabel->setText(tr("%1 invites you to play %2 to %3.")
                                  .arg(invitation->opponentName(), game,
                                       tr("%n point(s)", "", invitation->matchLength())));
        ui.toasterButton->setText(tr("Accept"));
        ui.toasterButton->setStyleSheet(QStringLiteral(
            "QPushButton { border: 1px solid #199909; font-size: 12pt; color: white; min-height: 24px;"
            " border-radius: 6px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67,"
            " stop: 0 #22c70d, stop: 1 #116a06); }"
            "QPushButton:hover { border-color: #35d51f; }"
            "QPushButton:pressed { background-color: #116a06; }"));
        // The invitation was answered or withdrawn meanwhile.
        connect(invitation, &QObject::destroyed, this, &QWidget::hide);
        connect(invitation, &BackgammonGameWindow::summaryChanged, this, [this] {
            if (!m_invitation || m_invitation->phase() != BackgammonGameWindow::Offered)
                hide();
        });
    } else {
        ui.textLabel->setText(tr("Someone invites you to play Backgammon."));
        ui.toasterButton->setText(tr("Close preview"));
    }

    connect(ui.toasterButton, &QPushButton::clicked, this, &BackgammonToaster::accept);
    connect(ui.closeButton, &QPushButton::clicked, this, &QWidget::hide);
}

void BackgammonToaster::accept()
{
    if (m_invitation && m_invitation->phase() == BackgammonGameWindow::Offered)
        m_invitation->acceptInvitation();
    hide();
}
