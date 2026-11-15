/*******************************************************************************
 * gui/BackgammonSettingsPage.h                                                *
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

#include <retroshare-gui/configpage.h>

namespace Ui { class BackgammonSettingsPage; }

// Preferences -> Backgammon: the same settings as the standalone
// Backgammon Settings dialog (BackgammonSettingsWidget).
class BackgammonSettingsPage : public ConfigPage
{
    Q_OBJECT

public:
    explicit BackgammonSettingsPage(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~BackgammonSettingsPage() override;

    void load() override;
    QPixmap iconPixmap() const override;
    QString pageName() const override;
    QString helpText() const override;

private:
    Ui::BackgammonSettingsPage *ui;
};
