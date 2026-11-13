/*******************************************************************************
 * tests/core-test.cpp                                                         *
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

// Tests of the rules core. Build and run:
//   g++ -std=c++17 -O2 -I core tests/core-test.cpp core/BgRules.cpp core/BgMatch.cpp -o core-test && ./core-test

#include "BgMatch.h"

#include <cstdio>
#include <cstdlib>
#include <set>

using namespace bg;

#ifndef ROUNDS
#define ROUNDS 300   // random matches per variant
#endif

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; } } while (0)

static int total(const Position &p, Side s)
{
    int n = p.bar(s) + p.off(s);
    for (int i = 1; i <= 24; ++i) n += p.checkers(s, i);
    return n;
}

static std::set<std::string> finals(const Position &p, Side s, const std::vector<Turn> &turns)
{
    std::set<std::string> out;
    for (const Turn &t : turns) {
        Position q = p;
        q.apply(s, t);
        std::string key;
        for (int i = 1; i <= 24; ++i) key += std::to_string(q.checkersAt(White, i) - q.checkersAt(Black, i)) + ",";
        key += std::to_string(q.bar(White)) + "," + std::to_string(q.bar(Black));
        out.insert(key);
    }
    return out;
}

int main()
{
    // --- initial positions
    for (Variant v : { Variant::Backgammon, Variant::LongNardy }) {
        Position p = Position::initial(v);
        CHECK(total(p, White) == 15);
        CHECK(total(p, Black) == 15);
        CHECK(p.pipCount(White) == p.pipCount(Black));
    }
    CHECK(Position::initial(Variant::Backgammon).pipCount(White) == 167);
    CHECK(Position::initial(Variant::LongNardy).pipCount(White) == 360);

    // --- move text
    Turn t;
    CHECK(parseTurn("24/18 13/11", t) && t.size() == 2 && t[0] == (Move { 24, 18 }) && t[1] == (Move { 13, 11 }));
    CHECK(parseTurn("bar/20 6/off", t) && t[0].from == BarPoint && t[1].to == OffPoint);
    CHECK(parseTurn("", t) && t.empty());
    CHECK(!parseTurn("25/1", t));
    CHECK(!parseTurn("x", t));
    CHECK(toString(Turn { { 24, 18 }, { BarPoint, 20 }, { 6, OffPoint } }) == "24/18 bar/20 6/off");

    // --- opening moves, backgammon: known numbers of different positions
    {
        Position p = Position::initial(Variant::Backgammon);
        auto t31 = legalTurns(p, White, { 3, 1 });
        auto f31 = finals(p, White, t31);
        std::printf("3-1: %zu turns, %zu positions\n", t31.size(), f31.size());
        CHECK(f31.size() == 16);
        auto t66 = legalTurns(p, White, { 6, 6 });
        auto f66 = finals(p, White, t66);
        std::printf("6-6: %zu turns, %zu positions\n", t66.size(), f66.size());
        for (const Turn &x : t31) CHECK(x.size() == 2);
        for (const Turn &x : t66) CHECK(x.size() == 4);
        // every opening roll can be played in full
        for (int a = 1; a <= 6; ++a)
            for (int b = 1; b <= a; ++b)
                for (const Turn &x : legalTurns(p, White, { a, b }))
                    CHECK(x.size() == (a == b ? 4u : 2u));
    }

    // --- specific rules (White numbering; black checkers negative)
    {
        // Bar: black holds White's 19 point, 20 is open: only the 5 enters,
        // and nothing else may move while a checker is on the bar.
        std::array<int, 25> pts {};
        pts[19] = -2; pts[6] = 5; pts[18] = -2;
        Position p = Position::custom(Variant::Backgammon, pts, 1, 0);
        auto turns = legalTurns(p, White, { 6, 5 });
        CHECK(turns.size() >= 1);
        for (const Turn &x : turns) CHECK(x.front() == (Move { BarPoint, 20 }));
        // 20/14 with the 6 after entering, or 6/0? not home: must play 6 elsewhere
        for (const Turn &x : turns) CHECK(x.size() == 2);

        // Both blocked on the bar: no move at all.
        pts[20] = -2;
        p = Position::custom(Variant::Backgammon, pts, 1, 0);
        turns = legalTurns(p, White, { 6, 5 });
        CHECK(turns.size() == 1 && turns[0].empty());

        // Larger die: 24/18 or 24/19 alone, never both -> must play the 6.
        std::array<int, 25> q {};
        q[24] = 1; q[13] = -2; q[12] = -2;   // 13 and 12 blocked
        p = Position::custom(Variant::Backgammon, q);
        turns = legalTurns(p, White, { 6, 5 });
        CHECK(turns.size() == 1 && turns[0] == (Turn { { 24, 18 } }));
        // If only the smaller die can be played, it is played.
        q[18] = -2;
        p = Position::custom(Variant::Backgammon, q);
        turns = legalTurns(p, White, { 6, 5 });
        CHECK(turns.size() == 1 && turns[0] == (Turn { { 24, 19 } }));

        // Bearing off with a larger number only from the highest point.
        std::array<int, 25> r {};
        r[5] = 1; r[3] = 1; r[24] = -1;
        p = Position::custom(Variant::Backgammon, r);
        turns = legalTurns(p, White, { 6, 1 });
        bool sixFromFive = false, sixFromThree = false;
        for (const Turn &x : turns)
            for (const Move &m : x) {
                if (m == (Move { 5, OffPoint })) sixFromFive = true;
                if (m == (Move { 3, OffPoint }) && x.front() == m) sixFromThree = true;
            }
        CHECK(sixFromFive && !sixFromThree);
        // Not all home: no bearing off.
        r[8] = 1;
        p = Position::custom(Variant::Backgammon, r);
        for (const Turn &x : legalTurns(p, White, { 6, 5 }))
            for (const Move &m : x) CHECK(m.to != OffPoint || x.front() != m || m.from != 8);

        // Gammon and backgammon values.
        std::array<int, 25> g {};
        g[24] = -15;  // black never left its home side... all 15 on White's 24 = black's 1
        p = Position::custom(Variant::Backgammon, g);
        CHECK(gameValue(p, White) == 2);
        g[24] = -14; g[3] = -1;  // one black checker in White's home board
        p = Position::custom(Variant::Backgammon, g);
        CHECK(gameValue(p, White) == 3);
        g[3] = 0; g[24] = -14;
        p = Position::custom(Variant::Backgammon, g, 0, 1);  // black on the bar
        CHECK(gameValue(p, White) == 3);
        g[24] = -13;
        p = Position::custom(Variant::Backgammon, g);  // black bore off one
        CHECK(gameValue(p, White) == 1);

        // Long Nardy: a single checker blocks, only one checker leaves the head.
        Position n = Position::initial(Variant::LongNardy);
        for (const Turn &x : legalTurns(n, White, { 5, 2 })) {
            int head = 0;
            for (const Move &m : x) head += m.from == 24;
            CHECK(head == 1);
        }
        // 6-6 on the first turn: two checkers from the head to 18.
        for (const Turn &x : legalTurns(n, White, { 6, 6 }, 2))
            CHECK(x == (Turn { { 24, 18 }, { 24, 18 } }));
        // Black's head is White's 12: a White checker on 18 cannot move 6.
        std::array<int, 25> nb {};
        nb[18] = 1; nb[24] = 14; nb[12] = -15;
        Position nn = Position::custom(Variant::LongNardy, nb);
        for (const Turn &x : legalTurns(nn, White, { 6, 3 }))
            for (const Move &m : x) CHECK(!(m.from == 18 && m.to == 12));
        // Six points in a row in front of every opposing checker are refused.
        std::array<int, 25> six {};
        for (int i = 18; i <= 22; ++i) six[i] = 1;   // 18..22 held
        six[24] = 10;
        six[12] = -15;                                // black head, behind the block
        Position sx = Position::custom(Variant::LongNardy, six);
        for (const Turn &x : legalTurns(sx, White, { 1, 2 })) {
            Position after = sx; after.apply(White, x);
            bool run = true;
            for (int i = 18; i <= 23; ++i) run = run && after.checkers(White, i);
            CHECK(!run);   // 24/23 would complete 18..23
        }
    }

    // --- random games: invariants and termination
    for (Variant v : { Variant::Backgammon, Variant::LongNardy }) {
        std::srand(1);
        int games = 0, maxTurns = 0, doubles = 0;
        for (int round = 0; round < ROUNDS; ++round) {
            Match match(v, 5);
            uint32_t seed = 1000 + round;
            while (match.phase() != Match::Phase::MatchOver) {
                if (match.phase() == Match::Phase::GameOver || match.gameIndex() < 0) {
                    match.startGame(seed++);
                    ++games;
                    continue;
                }
                const Side s = match.onRoll();
                if (match.phase() == Match::Phase::Rolling) {
                    if (match.canDouble(s) && std::rand() % 20 == 0) {
                        CHECK(match.offerDouble(s));
                        ++doubles;
                        if (std::rand() % 3) CHECK(match.take(opponent(s)));
                        else CHECK(match.drop(opponent(s)));
                        continue;
                    }
                    CHECK(match.roll(s));
                    CHECK(!match.roll(s));
                    continue;
                }
                const auto &legal = match.legalTurns();
                CHECK(!legal.empty());
                const Turn turn = legal[std::rand() % legal.size()];
                // a wrong side or an illegal turn is refused
                CHECK(!match.play(opponent(s), turn));
                Turn bogus { { 1, 24 } };
                if (!match.cannotMove()) CHECK(!match.play(s, bogus));
                CHECK(match.play(s, turn));
                const Position &p = match.position();
                CHECK(total(p, White) == 15 && total(p, Black) == 15);
                for (int pt = 1; pt <= 24; ++pt)
                    CHECK(!(p.checkersAt(White, pt) && p.checkersAt(Black, pt)));
                if (v == Variant::LongNardy) CHECK(p.bar(White) == 0 && p.bar(Black) == 0);
                maxTurns = std::max(maxTurns, match.ply());
                if (match.ply() > 2000) { CHECK(false); break; }
            }
            CHECK(match.score(White) >= 5 || match.score(Black) >= 5);
        }
        std::printf("%s: %d games, longest %d turns, %d doubles\n",
                    v == Variant::Backgammon ? "backgammon" : "long nardy", games, maxTurns, doubles);
    }

    // --- same seed, same game
    {
        Match a(Variant::Backgammon, 3), b(Variant::Backgammon, 3);
        a.startGame(42); b.startGame(42);
        CHECK(a.onRoll() == b.onRoll() && a.dice() == b.dice());
        for (int i = 0; i < 40 && a.phase() != Match::Phase::GameOver && a.phase() != Match::Phase::MatchOver; ++i) {
            if (a.phase() == Match::Phase::Rolling) { a.roll(a.onRoll()); b.roll(b.onRoll()); }
            CHECK(a.dice() == b.dice());
            Turn turn = a.legalTurns().back();
            CHECK(a.play(a.onRoll(), turn));
            CHECK(b.play(b.onRoll(), turn));
            CHECK(a.position() == b.position());
        }
    }

    // --- cube, crawford, scoring
    {
        Match m(Variant::Backgammon, 3);
        m.startGame(7);
        CHECK(!m.canDouble(m.onRoll()));   // opening roll already made: Moving
        CHECK(m.play(m.onRoll(), m.legalTurns().front()));
        Side s = m.onRoll();
        CHECK(m.canDouble(s));
        CHECK(!m.canDouble(opponent(s)));
        CHECK(m.offerDouble(s));
        CHECK(!m.take(s));
        CHECK(m.take(opponent(s)));
        CHECK(m.cube() == 2 && *m.cubeOwner() == opponent(s));
        CHECK(!m.canDouble(s));          // cube now belongs to the opponent
        CHECK(m.roll(s));
        CHECK(m.play(s, m.legalTurns().front()));
        Side o = m.onRoll();             // owner, may redouble
        CHECK(m.canDouble(o));
        CHECK(m.offerDouble(o));
        CHECK(m.drop(opponent(o)));      // dropping a 4 cube gives 2 points
        CHECK(m.phase() == Match::Phase::GameOver);
        CHECK(m.score(o) == 2 && m.lastResult().dropped);
        CHECK(m.crawfordGame());         // 2 of 3: next game is Crawford
        m.startGame(8);
        CHECK(m.play(m.onRoll(), m.legalTurns().front()));
        CHECK(!m.canDouble(m.onRoll()));
        Match one(Variant::Backgammon, 1);
        one.startGame(1);
        one.play(one.onRoll(), one.legalTurns().front());
        CHECK(!one.canDouble(one.onRoll()));   // no cube in a 1-point match
    }

    std::printf(failures ? "%d FAILURES\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
