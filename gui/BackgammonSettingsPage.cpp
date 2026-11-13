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

#include "BackgammonSettings.h"
#include "BackgammonSounds.h"
#include "BoardTheme.h"
#include "BackgammonConstants.h"

#include "gui/settings/rsharesettings.h"

#include <QSignalBlocker>

BackgammonSettingsPage::BackgammonSettingsPage(QWidget *parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags),
      ui(new Ui::BackgammonSettingsPage)
{
    ui->setupUi(this);

    for (const BoardTheme &theme : BoardTheme::all())
        ui->themeCombo->addItem(theme.name, theme.id);

    for (int i = 0; i < BackgammonSounds::EventCount; ++i) {
        auto *item = new QListWidgetItem(BackgammonSounds::label(BackgammonSounds::Event(i)),
                                         ui->eventsList);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, i);
        item->setCheckState(Qt::Checked);
    }

    connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BackgammonSettingsPage::themeChanged);
    connect(ui->openWindowCheck, &QCheckBox::toggled, this, [](bool on) { BackgammonSettings::setOpenInvitationWindow(on); });
    connect(ui->toasterCheck, &QCheckBox::toggled, this, [](bool on) { BackgammonSettings::setInvitationToaster(on); });
    connect(ui->soundsGroup, &QGroupBox::toggled, this, &BackgammonSettingsPage::soundsToggled);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &BackgammonSettingsPage::volumeChanged);
    connect(ui->eventsList, &QListWidget::itemChanged, this, &BackgammonSettingsPage::eventChanged);
    connect(ui->eventsList, &QListWidget::itemDoubleClicked, this, &BackgammonSettingsPage::eventActivated);
}

BackgammonSettingsPage::~BackgammonSettingsPage()
{
    delete ui;
}

void BackgammonSettingsPage::load()
{
    const QSignalBlocker blockGroup(ui->soundsGroup);
    const QSignalBlocker blockSlider(ui->volumeSlider);
    const QSignalBlocker blockList(ui->eventsList);
    const QSignalBlocker blockTheme(ui->themeCombo);
    const QSignalBlocker blockWindow(ui->openWindowCheck);
    const QSignalBlocker blockToaster(ui->toasterCheck);

    ui->openWindowCheck->setChecked(BackgammonSettings::openInvitationWindow());
    ui->toasterCheck->setChecked(BackgammonSettings::invitationToaster());

    ui->themeCombo->setCurrentIndex(qMax(0, ui->themeCombo->findData(BoardTheme::current().id)));
    updatePreview();

    ui->soundsGroup->setChecked(BackgammonSounds::soundsEnabled());
    ui->volumeSlider->setValue(BackgammonSounds::volume());
    ui->volumeValue->setText(tr("%1%").arg(BackgammonSounds::volume()));
    for (int row = 0; row < ui->eventsList->count(); ++row) {
        QListWidgetItem *item = ui->eventsList->item(row);
        const auto event = BackgammonSounds::Event(item->data(Qt::UserRole).toInt());
        item->setCheckState(BackgammonSounds::eventEnabled(event) ? Qt::Checked : Qt::Unchecked);
    }
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
    return tr("Choose the board colours and which Backgammon game events play a sound.");
}

void BackgammonSettingsPage::themeChanged(int index)
{
    const QString id = ui->themeCombo->itemData(index).toString();
    BoardTheme::setCurrent(id);
    Settings->setValueToGroup(QStringLiteral("Backgammon"), QStringLiteral("BoardTheme"), id);
    updatePreview();
}

void BackgammonSettingsPage::updatePreview()
{
    const int index = qMax(0, ui->themeCombo->currentIndex());
    ui->themePreview->setPixmap(BoardTheme::all().at(index).preview(QSize(320, 150)));
}

void BackgammonSettingsPage::soundsToggled(bool enabled)
{
    BackgammonSounds::setSoundsEnabled(enabled);
}

void BackgammonSettingsPage::volumeChanged(int value)
{
    BackgammonSounds::setVolume(value);
    ui->volumeValue->setText(tr("%1%").arg(value));
}

void BackgammonSettingsPage::eventChanged(QListWidgetItem *item)
{
    const auto event = BackgammonSounds::Event(item->data(Qt::UserRole).toInt());
    BackgammonSounds::setEventEnabled(event, item->checkState() == Qt::Checked);
}

void BackgammonSettingsPage::eventActivated(QListWidgetItem *item)
{
    // Preview, also when that sound is switched off.
    BackgammonSounds::preview(BackgammonSounds::Event(item->data(Qt::UserRole).toInt()));
}
