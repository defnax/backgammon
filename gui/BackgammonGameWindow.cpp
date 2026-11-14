/*******************************************************************************
 * gui/BackgammonGameWindow.cpp                                                *
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

#include "BackgammonGameWindow.h"
#include "ui_BackgammonGameWindow.h"

#include "BackgammonConstants.h"
#include "BackgammonSounds.h"
#include "BoardWidget.h"

#include "gui/gxs/GxsIdDetails.h"
#include <retroshare/rsidentity.h>

#include <QApplication>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QTimer>
#include <QtEndian>

namespace {
constexpr int PassDelayMs = 1500;   // a roll that cannot be played is shown, then passed
}

BackgammonGameWindow::BackgammonGameWindow(const RsGxsId &peer, const RsGxsId &local,
                                           bg::Variant variant, int length, QWidget *parent)
    : QWidget(parent, Qt::Window),
      ui(new Ui::BackgammonGameWindow),
      m_peer(peer),
      m_local(local),
      m_variant(variant),
      m_length(length)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(ICON_PATH));

    m_opponentName = QString::fromStdString(peer.toStdString());
    ui->opponentLabel->setText(m_opponentName);
    ui->avatarLabel->setPixmap(GxsIdDetails::makeDefaultIcon(peer));
    GxsIdDetails::process(peer, [](GxsIdDetailsType type, const RsIdentityDetails &details,
                                   QObject *target, const QVariant &) {
        if (type != GXS_ID_DETAILS_TYPE_DONE)
            return;
        auto *window = static_cast<BackgammonGameWindow *>(target);
        QPixmap avatar;
        if (details.mAvatar.mSize)
            GxsIdDetails::loadPixmapFromData(details.mAvatar.mData, details.mAvatar.mSize, avatar);
        window->setOpponentDetails(QString::fromUtf8(details.mNickname.c_str()), avatar);
    }, this);

    ui->resultLabel->hide();
    ui->cubeOfferFrame->hide();
    ui->board->setPosition(bg::Position::initial(variant));

    connect(ui->acceptButton, &QPushButton::clicked, this, &BackgammonGameWindow::acceptRequested);
    connect(ui->declineButton, &QPushButton::clicked, this, &BackgammonGameWindow::onDeclineClicked);
    connect(ui->rollButton, &QPushButton::clicked, this, &BackgammonGameWindow::onRollClicked);
    connect(ui->endTurnButton, &QPushButton::clicked, this, &BackgammonGameWindow::onEndTurnClicked);
    connect(ui->undoButton, &QPushButton::clicked, this, &BackgammonGameWindow::onUndoClicked);
    connect(ui->doubleButton, &QPushButton::clicked, this, &BackgammonGameWindow::onDoubleClicked);
    connect(ui->takeButton, &QPushButton::clicked, this, &BackgammonGameWindow::onTakeClicked);
    connect(ui->dropButton, &QPushButton::clicked, this, &BackgammonGameWindow::onDropClicked);
    connect(ui->board, &BoardWidget::movesChanged, this, &BackgammonGameWindow::updateButtons);
    connect(ui->board, &BoardWidget::rollRequested, this, &BackgammonGameWindow::onRollClicked);
    connect(ui->board, &BoardWidget::doubleRequested, this, &BackgammonGameWindow::onDoubleClicked);
    connect(ui->board, &BoardWidget::endTurnRequested, this, &BackgammonGameWindow::onEndTurnClicked);
    connect(ui->board, &BoardWidget::checkerMoved, this, [] {
        BackgammonSounds::play(BackgammonSounds::Checker);
    });

    ui->lbMatch->setText(tr("First to %n point(s) wins", "", m_length));
    refresh();
    updateButtons();
}

BackgammonGameWindow::~BackgammonGameWindow()
{
    delete ui;
}

/* ---------------------------------------------------------------------- */
/* Header                                                                  */
/* ---------------------------------------------------------------------- */

void BackgammonGameWindow::setOpponentDetails(const QString &name, const QPixmap &avatar)
{
    if (!name.isEmpty()) {
        m_opponentName = name;
        ui->opponentLabel->setText(name);
    }
    if (!avatar.isNull())
        ui->avatarLabel->setPixmap(avatar);
    if (m_match)
        syncBoard();
    refresh();
}

QString BackgammonGameWindow::sideName(bg::Side side) const
{
    return side == localSide() ? tr("You") : m_opponentName;
}

QString BackgammonGameWindow::summary() const
{
    const QString variant = m_variant == bg::Variant::LongNardy ? tr("Long Nardy") : tr("Backgammon");
    return tr("%1 vs %2 — %3").arg(variant, m_opponentName, ui->statusLabel->text());
}

void BackgammonGameWindow::setPhase(Phase phase)
{
    if (m_phase == Ended)
        return;
    m_phase = phase;
    refresh();
    updateButtons();
}

void BackgammonGameWindow::setWaitingDetail(const QString &text)
{
    m_waitingDetail = text;
    refresh();
}

void BackgammonGameWindow::setLinkDown(bool down)
{
    if (m_linkDown == down)
        return;
    m_linkDown = down;
    refresh();
    updateButtons();
}

void BackgammonGameWindow::refresh()
{
    ui->acceptButton->setVisible(m_phase == Offered);
    ui->declineButton->setText(m_phase == Offered ? tr("Decline")
                             : m_phase == Playing ? tr("Resign")
                             : m_phase == Ended ? tr("Close") : tr("Cancel"));

    const QString variant = m_variant == bg::Variant::LongNardy ? tr("Long Nardy") : tr("Backgammon");
    const QString points = tr("%n point(s)", "", m_length);
    setWindowTitle(tr("%1 with %2").arg(variant, m_opponentName));

    if (m_phase == Ended)
        return; // finish() owns the final status text

    QString text;
    switch (m_phase) {
    case Connecting:
    case Waiting:
        text = m_waitingDetail.isEmpty() ? tr("Waiting for your opponent to accept") : m_waitingDetail;
        text += QStringLiteral(" (%1, %2)").arg(variant, points);
        break;
    case Offered:    text = tr("Invites you to play %1 to %2").arg(variant, points); break;
    case Confirming: text = tr("Starting the game…"); break;
    case Playing: {
        const bool mine = m_match && m_match->onRoll() == localSide();
        if (m_match && m_match->phase() == bg::Match::Phase::DoubleOffered)
            text = mine ? tr("Waiting for %1 to take or drop your double").arg(m_opponentName)
                        : tr("%1 doubles: take or drop?").arg(m_opponentName);
        else if (m_match && m_match->phase() == bg::Match::Phase::Rolling)
            text = mine ? tr("Your turn: roll the dice") : tr("%1 is rolling").arg(m_opponentName);
        else
            text = mine ? tr("Your turn: move your checkers") : tr("%1 is playing").arg(m_opponentName);
        break;
    }
    case Ended: break;
    }
    if (m_linkDown)
        text = tr("Connection lost — reconnecting…");
    ui->statusLabel->setText(text);
    emit summaryChanged();
}

/* ---------------------------------------------------------------------- */
/* Match                                                                   */
/* ---------------------------------------------------------------------- */

uint32_t BackgammonGameWindow::seedFor(int gameIndex) const
{
    const QByteArray digest = QCryptographicHash::hash(
        m_seed + "/game/" + QByteArray::number(gameIndex), QCryptographicHash::Sha256);
    return qFromBigEndian<quint32>(digest.constData());
}

void BackgammonGameWindow::acceptInvitation()
{
    if (m_phase != Offered)
        return;
    showNormal();
    raise();
    activateWindow();
    emit acceptRequested();
}

void BackgammonGameWindow::declineInvitation()
{
    if (m_phase == Offered)
        close();   // closing sends the decline
}

void BackgammonGameWindow::startMatch(const QByteArray &diceSeed, bool localIsWhite)
{
    if (m_match)
        return;
    m_seed = diceSeed;
    m_localWhite = localIsWhite;
    m_match.reset(new bg::Match(m_variant, m_length));
    ui->board->setLocalSide(localSide());

    setPhase(Playing);
    BackgammonSounds::play(BackgammonSounds::GameStart);
    log(tr("You play %1.").arg(m_localWhite ? tr("white") : tr("black")));
    startNextGame();
}

void BackgammonGameWindow::startNextGame()
{
    m_match->startGame(seedFor(m_match->gameIndex() + 1));
    log(tr("— Game %1 —").arg(m_match->gameIndex() + 1));
    if (m_match->crawfordGame())
        log(tr("Crawford game: no doubling"));

    const auto opening = m_match->openingDice();
    log(tr("Opening roll: %1 %2, %3 %4. %5 starts.")
            .arg(sideName(bg::White)).arg(opening.first)
            .arg(sideName(bg::Black)).arg(opening.second)
            .arg(sideName(m_match->onRoll())));
    if (m_match->phase() == bg::Match::Phase::Moving)
        announceRoll(m_match->onRoll());
    syncBoard();
}

void BackgammonGameWindow::syncBoard()
{
    if (!m_match)
        return;
    const bg::Match &match = *m_match;
    BoardWidget *board = ui->board;

    board->setPosition(match.position());
    board->setDice(match.dice(), match.onRoll());
    board->setCube(match.cube(), match.cubeOwner(),
                   m_variant == bg::Variant::Backgammon && m_length > 1);
    const bool myMove = m_phase == Playing && match.phase() == bg::Match::Phase::Moving
                        && match.onRoll() == localSide();
    board->setLegalTurns(myMove ? match.legalTurns() : std::vector<bg::Turn>());

    ui->lbWhite->setText(m_localWhite ? tr("White (you)") : tr("White (%1)").arg(m_opponentName));
    ui->lbBlack->setText(m_localWhite ? tr("Black (%1)").arg(m_opponentName) : tr("Black (you)"));
    ui->lbScoreWhite->setText(QString::number(match.score(bg::White)));
    ui->lbScoreBlack->setText(QString::number(match.score(bg::Black)));
    ui->lbPipsWhite->setText(QString::number(match.position().pipCount(bg::White)));
    ui->lbPipsBlack->setText(QString::number(match.position().pipCount(bg::Black)));

    refresh();
    updateButtons();

    if (myMove)
        QApplication::alert(this);
    // Nothing can be played: show the roll a moment, then pass.
    if (myMove && match.cannotMove()) {
        const int game = match.gameIndex(), turn = match.ply();
        QTimer::singleShot(PassDelayMs, this, [this, game, turn] {
            if (m_match && m_match->gameIndex() == game && m_match->ply() == turn
                    && m_match->phase() == bg::Match::Phase::Moving && m_match->onRoll() == localSide())
                onEndTurnClicked();
        });
    }
}

void BackgammonGameWindow::updateButtons()
{
    const bool playing = m_phase == Playing && m_match && !m_linkDown;
    const bool mine = playing && m_match->onRoll() == localSide();
    const auto phase = m_match ? m_match->phase() : bg::Match::Phase::GameOver;

    const bool cube = m_variant == bg::Variant::Backgammon && m_length > 1;
    ui->doubleButton->setVisible(cube);
    ui->doubleButton->setEnabled(mine && m_match->canDouble(localSide()));
    ui->rollButton->setEnabled(mine && phase == bg::Match::Phase::Rolling);
    ui->undoButton->setEnabled(mine && phase == bg::Match::Phase::Moving && ui->board->canUndo());
    ui->endTurnButton->setEnabled(mine && phase == bg::Match::Phase::Moving
                                  && (ui->board->turnComplete() || m_match->cannotMove()));
    ui->board->setRollPrompt(ui->rollButton->isEnabled(), ui->doubleButton->isEnabled() && cube);
}

void BackgammonGameWindow::log(const QString &text)
{
    ui->logList->addItem(text);
    ui->logList->scrollToBottom();
}

void BackgammonGameWindow::announceRoll(bg::Side side)
{
    const auto &dice = m_match->dice();
    if (dice.size() < 2)
        return;
    log(tr("%1 rolled %2-%3").arg(sideName(side)).arg(dice[0]).arg(dice[1]));
    BackgammonSounds::play(BackgammonSounds::Roll);
    if (m_match->cannotMove()) {
        log(tr("%1 cannot move").arg(sideName(side)));
        QTimer::singleShot(700, this, [] { BackgammonSounds::play(BackgammonSounds::CannotMove); });
    }
}

void BackgammonGameWindow::afterAction()
{
    const auto phase = m_match->phase();
    if (phase != bg::Match::Phase::GameOver && phase != bg::Match::Phase::MatchOver) {
        syncBoard();
        return;
    }

    const auto &result = m_match->lastResult();
    const bool won = result.winner == localSide();
    QString how;
    if (result.dropped)
        how = tr("double dropped");
    else if (result.points / m_match->cube() == 3)
        how = tr("backgammon");
    else if (result.points / m_match->cube() == 2)
        how = m_variant == bg::Variant::LongNardy ? tr("mars") : tr("gammon");
    log(tr("%1 won the game: %n point(s)", "", result.points).arg(won ? tr("You") : m_opponentName)
        + (how.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(how)));
    syncBoard();

    if (phase == bg::Match::Phase::MatchOver) {
        const int mine = m_match->score(localSide()), theirs = m_match->score(remoteSide());
        BackgammonSounds::play(won ? BackgammonSounds::MatchWon : BackgammonSounds::MatchLost);
        finish(won ? tr("You won the match %1–%2!").arg(mine).arg(theirs)
                   : tr("You lost the match %1–%2").arg(mine).arg(theirs));
        return;
    }
    BackgammonSounds::play(won ? BackgammonSounds::GameWon : BackgammonSounds::GameLost);
    startNextGame();
}

/* ---------------------------------------------------------------------- */
/* Local actions                                                           */
/* ---------------------------------------------------------------------- */

void BackgammonGameWindow::onRollClicked()
{
    if (m_phase != Playing || !m_match)
        return;
    const int game = m_match->gameIndex(), turn = m_match->ply();
    if (!m_match->roll(localSide()))
        return;
    emit diceToSend(game, turn);
    announceRoll(localSide());
    syncBoard();
}

void BackgammonGameWindow::onEndTurnClicked()
{
    if (m_phase != Playing || !m_match)
        return;
    const bg::Turn turn = m_match->cannotMove() ? bg::Turn() : ui->board->playedMoves();
    const int game = m_match->gameIndex(), ply = m_match->ply();
    if (!m_match->play(localSide(), turn))
        return;
    const QString moves = QString::fromStdString(bg::toString(turn));
    emit turnToSend(game, ply, moves);
    if (!turn.empty())
        log(tr("You: %1").arg(moves));
    afterAction();
}

void BackgammonGameWindow::onUndoClicked()
{
    ui->board->undo();
}

void BackgammonGameWindow::onDoubleClicked()
{
    if (m_phase != Playing || !m_match || !m_match->offerDouble(localSide()))
        return;
    log(tr("You double to %1").arg(2 * m_match->cube()));
    BackgammonSounds::play(BackgammonSounds::DoubleOffered);
    emit doubleOffered(m_match->gameIndex(), m_match->ply());
    syncBoard();
}

void BackgammonGameWindow::onTakeClicked()
{
    if (!m_match || !m_match->take(localSide()))
        return;
    ui->cubeOfferFrame->hide();
    emit doubleAnswered(true, m_match->gameIndex(), m_match->ply());
    log(tr("You take, the cube is at %1").arg(m_match->cube()));
    BackgammonSounds::play(BackgammonSounds::DoubleTaken);
    syncBoard();
}

void BackgammonGameWindow::onDropClicked()
{
    if (!m_match || m_match->phase() != bg::Match::Phase::DoubleOffered || m_match->onRoll() == localSide())
        return;
    ui->cubeOfferFrame->hide();
    // Answer first: dropping ends the game here and starts the next one.
    emit doubleAnswered(false, m_match->gameIndex(), m_match->ply());
    m_match->drop(localSide());
    log(tr("You drop"));
    BackgammonSounds::play(BackgammonSounds::DoubleDropped);
    afterAction();
}

/* ---------------------------------------------------------------------- */
/* Opponent's actions                                                      */
/* ---------------------------------------------------------------------- */

void BackgammonGameWindow::remoteDice(int gameIndex, int ply)
{
    if (m_phase != Playing || !m_match || gameIndex != m_match->gameIndex() || ply != m_match->ply())
        return;
    if (!m_match->roll(remoteSide()))
        return;
    announceRoll(remoteSide());
    syncBoard();
}

void BackgammonGameWindow::remoteTurn(int gameIndex, int ply, const QString &moves)
{
    if (m_phase != Playing || !m_match || gameIndex != m_match->gameIndex() || ply != m_match->ply()
            || m_match->onRoll() != remoteSide())
        return;

    // The dice packet may have been lost: the dice are known anyway.
    if (m_match->phase() == bg::Match::Phase::Rolling && m_match->roll(remoteSide()))
        announceRoll(remoteSide());

    bg::Turn turn;
    if (!bg::parseTurn(moves.toStdString(), turn) || !m_match->play(remoteSide(), turn)) {
        finish(tr("Your opponent sent a move that is not legal. The game was stopped."));
        return;
    }
    if (!turn.empty()) {
        log(tr("%1: %2").arg(m_opponentName, moves));
        BackgammonSounds::play(BackgammonSounds::OpponentMove);
    }
    afterAction();
}

bool BackgammonGameWindow::waitingForDoubleAnswer() const
{
    return m_match && m_match->phase() == bg::Match::Phase::DoubleOffered
        && m_match->onRoll() == localSide();
}

void BackgammonGameWindow::remoteDouble(int gameIndex, int ply)
{
    if (m_phase != Playing || !m_match || gameIndex != m_match->gameIndex() || ply != m_match->ply())
        return;
    if (!m_match->offerDouble(remoteSide()))
        return;
    log(tr("%1 doubles to %2").arg(m_opponentName).arg(2 * m_match->cube()));
    ui->cubeOfferLabel->setText(tr("%1 doubles to %2. Take and play on for %2 points, or drop and lose %3?")
                                    .arg(m_opponentName)
                                    .arg(2 * m_match->cube())
                                    .arg(tr("%n point(s)", "", m_match->cube())));
    ui->cubeOfferFrame->show();
    BackgammonSounds::play(BackgammonSounds::DoubleOffered);
    QApplication::alert(this);
    syncBoard();
}

void BackgammonGameWindow::remoteDoubleAnswer(bool take, int gameIndex, int ply)
{
    if (m_phase != Playing || !waitingForDoubleAnswer()
            || gameIndex != m_match->gameIndex() || ply != m_match->ply())
        return;
    if (take) {
        m_match->take(remoteSide());
        log(tr("%1 takes, the cube is at %2").arg(m_opponentName).arg(m_match->cube()));
        BackgammonSounds::play(BackgammonSounds::DoubleTaken);
        syncBoard();
    } else {
        m_match->drop(remoteSide());
        log(tr("%1 drops").arg(m_opponentName));
        BackgammonSounds::play(BackgammonSounds::DoubleDropped);
        afterAction();
    }
}

/* ---------------------------------------------------------------------- */
/* End of the match                                                        */
/* ---------------------------------------------------------------------- */

void BackgammonGameWindow::finish(const QString &message)
{
    if (m_phase == Ended)
        return;
    m_phase = Ended;
    m_linkDown = false;
    ui->cubeOfferFrame->hide();
    ui->board->setLegalTurns({});

    ui->statusLabel->setText(message);
    ui->resultLabel->setText(message);
    ui->resultLabel->show();
    refresh();
    updateButtons();
    emit summaryChanged();
    QApplication::alert(this);
}

bool BackgammonGameWindow::confirmResign(const QString &question)
{
    return QMessageBox::question(this, windowTitle(), question,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}

void BackgammonGameWindow::onDeclineClicked()
{
    if (m_phase != Playing) {
        // Closing sends the decline or the cancellation (see closed()).
        close();
        return;
    }
    if (!confirmResign(tr("Resign this match?")) || m_phase != Playing)
        return;
    emit resignRequested();
    if (m_match)
        m_match->resign(localSide());
    BackgammonSounds::play(BackgammonSounds::Resign);
    finish(tr("You resigned"));
}

void BackgammonGameWindow::closeEvent(QCloseEvent *event)
{
    if (m_phase == Playing) {
        if (!confirmResign(tr("Leaving now resigns the match. Leave anyway?"))) {
            event->ignore();
            return;
        }
        if (m_phase == Playing) {
            emit resignRequested();
            if (m_match)
                m_match->resign(localSide());
            finish(tr("You resigned"));
        }
    }
    emit closed();
    QWidget::closeEvent(event);
}
