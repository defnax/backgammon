/*******************************************************************************
 * gui/BoardWidget.h                                                           *
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

#include "core/BgRules.h"

#include <QPointF>
#include <QRectF>
#include <QWidget>

#include <optional>
#include <vector>

class QPainter;

// The backgammon board, drawn to fit the widget (4:3, frame included).
//
// The local player always sits at the bottom with the home board on the
// right, so a point on the screen is the same number as the local player's
// own point. When legal turns are given, the local player moves checkers by
// dragging them or by clicking a checker and then its destination.
class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    void setLocalSide(bg::Side side);
    void setPosition(const bg::Position &position);
    // Dice on the table; empty to hide them. The roller's dice lie on the
    // right half for the local player, on the left for the opponent.
    void setDice(const std::vector<int> &dice, bg::Side roller);
    void setCube(int value, std::optional<bg::Side> owner, bool visible);

    // Lets the local player play one of these turns from the current
    // position. An empty list stops the input.
    void setLegalTurns(const std::vector<bg::Turn> &turns);
    const bg::Turn &playedMoves() const { return m_played; }
    bool turnComplete() const;
    bool canUndo() const { return !m_played.empty(); }
    void undo();

    // Buttons on the board itself, like online backgammon: "Roll" (and
    // "Double") while the local player has to roll, an undo button on the
    // bar while moving, and the dice confirm the turn once it is complete.
    void setRollPrompt(bool roll, bool canDouble);

    QSize sizeHint() const override { return QSize(660, 500); }
    QSize minimumSizeHint() const override { return QSize(330, 250); }
    bool hasHeightForWidth() const override { return false; }

signals:
    void movesChanged();
    void checkerMoved(bool hit);
    void rollRequested();
    void doubleRequested();
    void endTurnRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // Board geometry in board units: 16.5 x 12.5 with the frame.
    QRectF boardRect() const;           // widget pixels
    qreal unit() const;
    QPointF toPixels(qreal x, qreal y) const;
    QRectF pointRect(int slot) const;   // slot = local player's point 1..24
    QRectF barRect(bool top) const;
    QRectF trayRect(bool top) const;
    QPointF checkerCentre(int slot, int index) const;
    // Point under the mouse: 1..24, BarPoint, OffPoint or -1.
    int hitTest(const QPointF &pos) const;

    bg::Side sideAt(int slot) const;    // owner of the checkers on a slot
    int countAt(int slot) const;
    int absoluteOf(int slot) const { return m_shown.absolute(m_local, slot); }

    const std::vector<bg::Move> &nextMoves() const { return m_next; }
    // Recomputes m_next and m_complete after the legal turns or the played
    // moves changed (there can be thousands of legal turns: not per paint).
    void updateNextMoves();
    std::vector<int> destinations(int from) const;
    void play(const bg::Move &move);
    void select(int point);

    void drawFrame(QPainter &p) const;
    void drawPoints(QPainter &p) const;
    void drawCheckers(QPainter &p) const;
    void drawChecker(QPainter &p, const QPointF &centre, qreal radius, bg::Side side, bool highlight) const;
    void drawTrays(QPainter &p) const;
    void drawDice(QPainter &p) const;
    void drawDie(QPainter &p, const QRectF &rect, int value, qreal opacity) const;
    void drawCube(QPainter &p) const;
    void drawOverlayButtons(QPainter &p) const;
    void drawPill(QPainter &p, const QRectF &rect, const QString &text, bool hover) const;

    enum Button { NoButton, RollButton, DoubleButton, UndoButton, ConfirmButton };
    QRectF buttonRect(Button button) const;
    QRectF diceArea() const;
    Button buttonAt(const QPointF &pos) const;

    bg::Side m_local = bg::White;
    bg::Position m_position;           // position before the moves being played
    bg::Position m_shown;              // m_position with m_played applied
    std::vector<int> m_dice;
    bg::Side m_roller = bg::White;
    int m_cubeValue = 1;
    std::optional<bg::Side> m_cubeOwner;
    bool m_cubeVisible = false;

    std::vector<bg::Turn> m_legal;
    bg::Turn m_played;
    std::vector<bg::Move> m_next;      // moves that can be played next
    bool m_complete = false;           // m_played is a whole legal turn
    int m_selected = -1;               // point of the picked-up checker
    bool m_dragging = false;
    QPointF m_dragPos;
    QPointF m_pressPos;
    bool m_rollPrompt = false;
    bool m_doublePrompt = false;
    Button m_hover = NoButton;
};
