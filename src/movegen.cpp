/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2023 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>

#include "movegen.h"
#include "position.h"
#include "uci.h"

namespace Stockfish {

namespace {

  template<Variant V, GenType Type, Direction D, bool Enemy>
  ExtMove* make_promotions(ExtMove* moveList, [[maybe_unused]] Square to) {

#ifdef ANTI
    if constexpr (V == ANTI_VARIANT)
    {
        if constexpr (Type == QUIETS || Type == CAPTURES || Type == NON_EVASIONS)
        {
            *moveList++ = make<PROMOTION>(to - D, to, QUEEN);
            *moveList++ = make<PROMOTION>(to - D, to, ROOK);
            *moveList++ = make<PROMOTION>(to - D, to, BISHOP);
            *moveList++ = make<PROMOTION>(to - D, to, KNIGHT);
            *moveList++ = make<PROMOTION>(to - D, to, KING);
        }
        return moveList;
    }
#endif
#ifdef LOSERS
    if constexpr (V == LOSERS_VARIANT)
    {
        if constexpr (Type == QUIETS || Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
        {
            *moveList++ = make<PROMOTION>(to - D, to, QUEEN);
            *moveList++ = make<PROMOTION>(to - D, to, ROOK);
            *moveList++ = make<PROMOTION>(to - D, to, BISHOP);
            *moveList++ = make<PROMOTION>(to - D, to, KNIGHT);
        }
        return moveList;
    }
#endif
#ifdef HELPMATE
    if constexpr (V == HELPMATE_VARIANT)
    {
        if constexpr (Type == QUIETS || Type == CAPTURES || Type == NON_EVASIONS)
        {
            *moveList++ = make<PROMOTION>(to - D, to, QUEEN);
            *moveList++ = make<PROMOTION>(to - D, to, ROOK);
            *moveList++ = make<PROMOTION>(to - D, to, BISHOP);
            *moveList++ = make<PROMOTION>(to - D, to, KNIGHT);
        }
        return moveList;
    }
#endif
    if constexpr (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
        *moveList++ = make<PROMOTION>(to - D, to, QUEEN);
        if constexpr (Enemy && Type == CAPTURES)
        {
            *moveList++ = make<PROMOTION>(to - D, to, ROOK);
            *moveList++ = make<PROMOTION>(to - D, to, BISHOP);
            *moveList++ = make<PROMOTION>(to - D, to, KNIGHT);
        }
    }

    if constexpr ((Type == QUIETS && !Enemy) || Type == EVASIONS || Type == NON_EVASIONS)
    {
        *moveList++ = make<PROMOTION>(to - D, to, ROOK);
        *moveList++ = make<PROMOTION>(to - D, to, BISHOP);
        *moveList++ = make<PROMOTION>(to - D, to, KNIGHT);
#ifdef EXTINCTION
        if constexpr (V == EXTINCTION_VARIANT)
            *moveList++ = make<PROMOTION>(to - D, to, KING);
#endif
    }

    return moveList;
  }

#ifdef CRAZYHOUSE
  template<Color Us, PieceType Pt, bool Checks>
  ExtMove* generate_drops(const Position& pos, ExtMove* moveList, Bitboard b) {
    if (pos.count_in_hand<Pt>(Us))
    {
        if (Checks)
            b &= pos.check_squares(Pt);
        while (b)
            *moveList++ = make_drop(pop_lsb(b), make_piece(Us, Pt));
    }

    return moveList;
  }
#endif

#if defined(ANTI) || defined(EXTINCTION) || defined(TWOKINGS)
  template<Variant V, Color Us, GenType Type>
  ExtMove* generate_king_moves(const Position& pos, ExtMove* moveList, Bitboard target) {
    Bitboard kings = pos.pieces(Us, KING);
    while (kings)
    {
        Square ksq = pop_lsb(kings);
        Bitboard b = attacks_bb<KING>(ksq) & target;
        while (b)
            *moveList++ = make_move(ksq, pop_lsb(b));
    }
    return moveList;
  }
#endif

  template<Variant V, Color Us, GenType Type>
  ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    constexpr Color     Them     = ~Us;
    constexpr Bitboard  TRank7BB = (Us == WHITE ? Rank7BB    : Rank2BB);
#ifdef HORDE
    constexpr Bitboard  TRank2BB = (Us == WHITE ? Rank2BB    : Rank7BB);
#endif
    constexpr Bitboard  TRank3BB = (Us == WHITE ? Rank3BB    : Rank6BB);
    constexpr Direction Up       = pawn_push(Us);
    constexpr Direction UpRight  = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction UpLeft   = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);

    const Bitboard emptySquares = ~pos.pieces();
    const Bitboard enemies      =  Type == EVASIONS ? pos.checkers()
                                                    : pos.pieces(Them);

    Bitboard pawnsOn7    = pos.pieces(Us, PAWN) &  TRank7BB;
    Bitboard pawnsNotOn7 = pos.pieces(Us, PAWN) & ~TRank7BB;

    // Single and double pawn pushes, no promotions
    if constexpr (Type != CAPTURES)
    {
        Bitboard b1 = shift<Up>(pawnsNotOn7)   & emptySquares;
#ifdef ANTI
        if constexpr (V == ANTI_VARIANT)
            b1 &= target;
#endif
        Bitboard b2 = shift<Up>(b1 & TRank3BB) & emptySquares;
#ifdef HORDE
        if constexpr (V == HORDE_VARIANT)
            b2 = shift<Up>(b1 & (TRank2BB | TRank3BB)) & emptySquares;
#endif

#ifdef LOSERS
        if constexpr (V == LOSERS_VARIANT)
        {
            b1 &= target;
            b2 &= target;
        }
#endif
        if constexpr (Type == EVASIONS) // Consider only blocking squares
        {
            b1 &= target;
            b2 &= target;
        }

        if constexpr (Type == QUIET_CHECKS) switch (V)
        {
#ifdef ANTI
        case ANTI_VARIANT:
        break;
#endif
#ifdef HORDE
        case HORDE_VARIANT:
            if (pos.is_horde_color(Them))
            {
                b1 = b2 = 0;
                break;
            }
        [[fallthrough]];
#endif
#ifdef PLACEMENT
        case CRAZYHOUSE_VARIANT:
            if (pos.is_placement() && pos.count_in_hand<KING>(Them))
                break;
        [[fallthrough]];
#endif
        default:
            // To make a quiet check, you either make a direct check by pushing a pawn
            // or push a blocker pawn that is not on the same file as the enemy king.
            // Discovered check promotion has been already generated amongst the captures.
            Square ksq = pos.square<KING>(Them);
            Bitboard dcCandidatePawns = pos.blockers_for_king(Them) & ~file_bb(ksq);
            b1 &= pawn_attacks_bb(Them, ksq) | shift<   Up>(dcCandidatePawns);
            b2 &= pawn_attacks_bb(Them, ksq) | shift<Up+Up>(dcCandidatePawns);
        }

        while (b1)
        {
            Square to = pop_lsb(b1);
            *moveList++ = make_move(to - Up, to);
        }

        while (b2)
        {
            Square to = pop_lsb(b2);
            *moveList++ = make_move(to - Up - Up, to);
        }
    }

    // Promotions and underpromotions
    if (pawnsOn7)
    {
        Bitboard b1 = shift<UpRight>(pawnsOn7) & enemies;
        Bitboard b2 = shift<UpLeft >(pawnsOn7) & enemies;
        Bitboard b3 = shift<Up     >(pawnsOn7) & emptySquares;

#ifdef ATOMIC
        if constexpr (V == ATOMIC_VARIANT)
        {
            //b1 &= (Type == CAPTURES || Type == NON_EVASIONS) ? target : ~adjacent_squares_bb(pos.pieces(Us, KING));
            //b2 &= (Type == CAPTURES || Type == NON_EVASIONS) ? target : ~adjacent_squares_bb(pos.pieces(Us, KING));
            if (Type == EVASIONS) {
                b1 = shift<UpRight>(pawnsOn7) & ~adjacent_squares_bb(pos.pieces(Us, KING)) & target & pos.pieces(Them);
                b2 = shift<UpLeft >(pawnsOn7) & ~adjacent_squares_bb(pos.pieces(Us, KING)) & target & pos.pieces(Them);
         }
            else if (Type == CAPTURES) {
                b1 = shift<UpRight>(pawnsOn7) & target;
                b2 = shift<UpLeft >(pawnsOn7) & target;
            }
            else if (Type == NON_EVASIONS) {
                b1 = shift<UpRight>(pawnsOn7) & target & pos.pieces(Them);
                b2 = shift<UpLeft >(pawnsOn7) & target & pos.pieces(Them);
            }
        }
#endif

#ifdef ATOMIC
            // Promotes only if promotion wins or explodes checkers
            if (V == ATOMIC_VARIANT && pos.checkers())
                b3 = shift<Up     >(pawnsOn7) & emptySquares & target;
#endif

#ifdef ANTI
        if constexpr (V == ANTI_VARIANT)
            b3 &= target;
#endif
#ifdef LOSERS
        if constexpr (V == LOSERS_VARIANT)
            b3 &= target;
#endif
        if constexpr (Type == EVASIONS)
            b3 &= target;

        while (b1)
            moveList = make_promotions<V, Type, UpRight, true>(moveList, pop_lsb(b1));

        while (b2)
            moveList = make_promotions<V, Type, UpLeft, true>(moveList, pop_lsb(b2));

        while (b3)
            moveList = make_promotions<V, Type, Up,    false>(moveList, pop_lsb(b3));
    }

    // Standard and en passant captures
    if constexpr (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
        Bitboard b1 = shift<UpRight>(pawnsNotOn7) & enemies;
        Bitboard b2 = shift<UpLeft >(pawnsNotOn7) & enemies;
#ifdef ATOMIC
        if constexpr (V == ATOMIC_VARIANT)
        {
            //b1 &= (Type == CAPTURES || Type == NON_EVASIONS) ? target : ~adjacent_squares_bb(pos.pieces(Us, KING));
            //b2 &= (Type == CAPTURES || Type == NON_EVASIONS) ? target : ~adjacent_squares_bb(pos.pieces(Us, KING));
            if (Type == EVASIONS) {
                b1 = shift<UpRight>(pawnsNotOn7) & ~adjacent_squares_bb(pos.pieces(Us, KING)) & target & pos.pieces(Them);
                b2 = shift<UpLeft >(pawnsNotOn7) & ~adjacent_squares_bb(pos.pieces(Us, KING)) & target & pos.pieces(Them);
         }
            else if (Type == CAPTURES) {
                b1 = shift<UpRight>(pawnsNotOn7) & target;
                b2 = shift<UpLeft >(pawnsNotOn7) & target;
            }
            else if (Type == NON_EVASIONS) {
                b1 = shift<UpRight>(pawnsNotOn7) & target & pos.pieces(Them);
                b2 = shift<UpLeft >(pawnsNotOn7) & target & pos.pieces(Them);
            }
        }
#endif 

        while (b1)
        {
            Square to = pop_lsb(b1);
            *moveList++ = make_move(to - UpRight, to);
        }

        while (b2)
        {
            Square to = pop_lsb(b2);
            *moveList++ = make_move(to - UpLeft, to);
        }

#ifdef KNIGHTRELAY
        if constexpr (V == KNIGHTRELAY_VARIANT)
            for (b1 = pos.pieces(Us, PAWN); b1; )
            {
                Square from = pop_lsb(b1);
                if ((b2 = attacks_bb<KNIGHT>(from)) & pos.pieces(Us, KNIGHT))
                    for (b2 &= target & ~(Rank1BB | Rank8BB); b2; )
                        *moveList++ = make_move(from, pop_lsb(b2));
            }
        else
#endif
        if (pos.ep_square() != SQ_NONE)
        {
            assert(rank_of(pos.ep_square()) == relative_rank(Us, RANK_6));

            // An en passant capture cannot resolve a discovered check
            if (Type == EVASIONS && (target & (pos.ep_square() + Up)))
                return moveList;

            b1 = pawnsNotOn7 & pawn_attacks_bb(Them, pos.ep_square());

            assert(b1);

            while (b1)
                *moveList++ = make<EN_PASSANT>(pop_lsb(b1), pos.ep_square());
        }
    }

    return moveList;
  }


  template<Variant V, Color Us, PieceType Pt, bool Checks>
  ExtMove* generate_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    static_assert(Pt != KING && Pt != PAWN, "Unsupported piece type in generate_moves()");

    Bitboard bb = pos.pieces(Us, Pt);

    while (bb)
    {
        Square from = pop_lsb(bb);
        Bitboard b = attacks_bb<Pt>(from, pos.pieces()) & target;
#ifdef KNIGHTRELAY
        if constexpr (V == KNIGHTRELAY_VARIANT)
        {
            if (Pt == KNIGHT)
                b &= ~pos.pieces();
            else if (attacks_bb<KNIGHT>(from) & pos.pieces(color_of(pos.piece_on(from)), KNIGHT))
                b |= attacks_bb<KNIGHT>(from) & target;
        }
#endif
#ifdef RELAY
        if constexpr (V == RELAY_VARIANT)
            for (PieceType pt = KNIGHT; pt <= KING; ++pt)
                if (attacks_bb(pt, from, pos.pieces()) & pos.pieces(color_of(pos.piece_on(from)), pt))
                    b |= attacks_bb(pt, from, pos.pieces()) & target;
#endif

        // To check, you either move freely a blocker or make a direct check.
        if (Checks && (Pt == QUEEN || !(pos.blockers_for_king(~Us) & from)))
            b &= pos.check_squares(Pt);

        while (b)
            *moveList++ = make_move(from, pop_lsb(b));
    }

    return moveList;
  }


  template<Variant V, Color Us, GenType Type>
  ExtMove* generate_all(const Position& pos, ExtMove* moveList) {

    static_assert(Type != LEGAL, "Unsupported type in generate_all()");

    constexpr bool Checks = Type == QUIET_CHECKS; // Reduce template instantiations
    Square ksq;
    switch (V)
    {
#ifdef EXTINCTION
    case EXTINCTION_VARIANT:
        ksq = pos.castling_king_square(Us);
    break;
#endif
#ifdef TWOKINGS
    case TWOKINGS_VARIANT:
        ksq = pos.castling_king_square(Us);
    break;
#endif
#ifdef HORDE
    case HORDE_VARIANT:
        if (pos.is_horde_color(Us))
        {
            ksq = SQ_NONE;
            break;
        }
    [[fallthrough]];
#endif
#ifdef GIVEAWAY
    case ANTI_VARIANT:
        if (pos.is_giveaway())
            ksq = pos.castling_king_square(Us);
    [[fallthrough]];
#endif
    default:
        ksq = pos.square<KING>(Us);
    }
    Bitboard target;

    // Skip generating non-king moves when in double check
    if (Type != EVASIONS || !more_than_one(pos.checkers()))
    {
        target = Type == EVASIONS     ?  between_bb(ksq, lsb(pos.checkers()))
               : Type == NON_EVASIONS ? ~pos.pieces( Us)
               : Type == CAPTURES     ?  pos.pieces(~Us)
                                      : ~pos.pieces(   ); // QUIETS || QUIET_CHECKS
#ifdef ANTI
        if (V == ANTI_VARIANT && pos.can_capture())
            target &= pos.pieces(~Us);
#endif
#ifdef ATOMIC
        if constexpr (V == ATOMIC_VARIANT)
        {
            // Captures that explode the opposing king or checking piece are legal.
            if (Type == EVASIONS)
                target |= pos.pieces(~Us) & adjacent_squares_bb(pos.checkers() | pos.square<KING>(~Us));
            target &= ~(pos.pieces(~Us) & adjacent_squares_bb(pos.pieces(Us, KING)));
        }
#endif
#ifdef LOSERS
        if (V == LOSERS_VARIANT && pos.can_capture_losers())
            target &= pos.pieces(~Us);
#endif

        moveList = generate_pawn_moves<V, Us, Type>(pos, moveList, target);
        moveList = generate_moves<V, Us, KNIGHT, Checks>(pos, moveList, target);
        moveList = generate_moves<V, Us, BISHOP, Checks>(pos, moveList, target);
        moveList = generate_moves<V, Us,   ROOK, Checks>(pos, moveList, target);
        moveList = generate_moves<V, Us,  QUEEN, Checks>(pos, moveList, target);
    }

#ifdef CRAZYHOUSE
    if (V == CRAZYHOUSE_VARIANT && Type != CAPTURES && pos.count_in_hand<ALL_PIECES>(Us))
    {
        if (Type == EVASIONS)
            target = between_bb(ksq, lsb(pos.checkers()));
        Bitboard b = Type == EVASIONS ? target ^ pos.checkers() :
                     Type == NON_EVASIONS ? target ^ pos.pieces(~Us) : target;
#ifdef PLACEMENT
        if (pos.is_placement())
            b &= (Us == WHITE ? Rank1BB : Rank8BB);
#endif
        moveList = generate_drops<Us,   PAWN, Checks>(pos, moveList, b & ~(Rank1BB | Rank8BB));
        moveList = generate_drops<Us, KNIGHT, Checks>(pos, moveList, b);
        moveList = generate_drops<Us, BISHOP, Checks>(pos, moveList, b);
        moveList = generate_drops<Us,   ROOK, Checks>(pos, moveList, b);
        moveList = generate_drops<Us,  QUEEN, Checks>(pos, moveList, b);
#ifdef PLACEMENT
        if (pos.is_placement())
            moveList = generate_drops<Us, KING, Checks>(pos, moveList, b);
#endif
    }
#ifdef PLACEMENT
    if (pos.is_placement() && pos.count_in_hand<ALL_PIECES>(Us))
        return moveList;
#endif
#endif

    switch (V)
    {
#ifdef ANTI
    case ANTI_VARIANT:
        if (Type == EVASIONS)
            target = between_bb(ksq, lsb(pos.checkers()));
        moveList = generate_king_moves<V, Us, Type>(pos, moveList, target);
        if (pos.can_capture())
            return moveList;
    break;
#endif
#ifdef EXTINCTION
    case EXTINCTION_VARIANT:
        if (Type == EVASIONS)
            target = between_bb(ksq, lsb(pos.checkers()));
        moveList = generate_king_moves<V, Us, Type>(pos, moveList, target);
    break;
#endif
#ifdef TWOKINGS
    case TWOKINGS_VARIANT:
        if (Type != EVASIONS)
            moveList = generate_king_moves<V, Us, Type>(pos, moveList, target);
    break;
#endif
#ifdef HORDE
    case HORDE_VARIANT:
        if (pos.is_horde_color(Us))
            return moveList;
    [[fallthrough]];
#endif
    default:
    if (!Checks || pos.blockers_for_king(~Us) & ksq)
    {
        Bitboard b = attacks_bb<KING>(ksq) & (Type == EVASIONS ? ~pos.pieces(Us) : target);
        if (Checks)
            b &= ~attacks_bb<QUEEN>(pos.square<KING>(~Us));
#ifdef RACE
        if constexpr (V == RACE_VARIANT)
        {
            // Early generate king advance moves
            if (Type == CAPTURES)
                b |= attacks_bb<KING>(ksq) & passed_pawn_span(WHITE, ksq) & ~pos.pieces();
            if (Type == QUIETS)
                b &= ~passed_pawn_span(WHITE, ksq);
        }
#endif
#ifdef RELAY
        if constexpr (V == RELAY_VARIANT)
        {
            if (Type == EVASIONS)
                target = between_bb(ksq, lsb(pos.checkers()));
            for (PieceType pt = KNIGHT; pt <= KING; ++pt)
                if (attacks_bb(pt, ksq, pos.pieces()) & pos.pieces(Us, pt))
                    b |= attacks_bb(pt, ksq, pos.pieces()) & target;
        }
#endif
#ifdef LOSERS
        if (V == LOSERS_VARIANT && pos.can_capture_losers())
            b &= pos.pieces(~Us);
#endif

        while (b)
            *moveList++ = make_move(ksq, pop_lsb(b));

#ifdef LOSERS
        if (V == LOSERS_VARIANT && pos.can_capture_losers()) {} else
#endif
        if ((Type == QUIETS || Type == NON_EVASIONS) && pos.can_castle(Us & ANY_CASTLING))
            for (CastlingRights cr : { Us & KING_SIDE, Us & QUEEN_SIDE } )
                if (!pos.castling_impeded(cr) && pos.can_castle(cr))
                    *moveList++ = make<CASTLING>(ksq, pos.castling_rook_square(cr));
    }
    }

    return moveList;
  }

} // namespace


/// <CAPTURES>     Generates all pseudo-legal captures plus queen promotions
/// <QUIETS>       Generates all pseudo-legal non-captures and underpromotions
/// <EVASIONS>     Generates all pseudo-legal check evasions when the side to move is in check
/// <QUIET_CHECKS> Generates all pseudo-legal non-captures giving check, except castling and promotions
/// <NON_EVASIONS> Generates all pseudo-legal captures and non-captures
///
/// Returns a pointer to the end of the move list.

template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList) {

  static_assert(Type != LEGAL, "Unsupported type in generate()");
  assert((Type == EVASIONS) == (bool)pos.checkers());

  Color us = pos.side_to_move();

  switch (pos.variant())
  {
#ifdef ANTI
  case ANTI_VARIANT:
      return us == WHITE ? generate_all<ANTI_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<ANTI_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef ATOMIC
  case ATOMIC_VARIANT:
      return us == WHITE ? generate_all<ATOMIC_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<ATOMIC_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef CRAZYHOUSE
  case CRAZYHOUSE_VARIANT:
      return us == WHITE ? generate_all<CRAZYHOUSE_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<CRAZYHOUSE_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef EXTINCTION
  case EXTINCTION_VARIANT:
      return us == WHITE ? generate_all<EXTINCTION_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<EXTINCTION_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef GRID
  case GRID_VARIANT:
      return us == WHITE ? generate_all<GRID_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<GRID_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef HELPMATE
  case HELPMATE_VARIANT:
      return us == WHITE ? generate_all<HELPMATE_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<HELPMATE_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef HORDE
  case HORDE_VARIANT:
      return us == WHITE ? generate_all<HORDE_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<HORDE_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef LOSERS
  case LOSERS_VARIANT:
      return us == WHITE ? generate_all<LOSERS_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<LOSERS_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef RACE
  case RACE_VARIANT:
      return us == WHITE ? generate_all<RACE_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<RACE_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef KNIGHTRELAY
  case KNIGHTRELAY_VARIANT:
      return us == WHITE ? generate_all<KNIGHTRELAY_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<KNIGHTRELAY_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef RELAY
  case RELAY_VARIANT:
      return us == WHITE ? generate_all<RELAY_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<RELAY_VARIANT, BLACK, Type>(pos, moveList);
#endif
#ifdef TWOKINGS
  case TWOKINGS_VARIANT:
      return us == WHITE ? generate_all<TWOKINGS_VARIANT, WHITE, Type>(pos, moveList)
                         : generate_all<TWOKINGS_VARIANT, BLACK, Type>(pos, moveList);
#endif
  default:
  return us == WHITE ? generate_all<CHESS_VARIANT, WHITE, Type>(pos, moveList)
                     : generate_all<CHESS_VARIANT, BLACK, Type>(pos, moveList);
  }
}

// Explicit template instantiations
template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<EVASIONS>(const Position&, ExtMove*);
template ExtMove* generate<QUIET_CHECKS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);


/// generate<LEGAL> generates all the legal moves in the given position

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList) {
  // Return immediately at end of variant
  if (pos.is_variant_end())
      return moveList;

  Color us = pos.side_to_move();
  Bitboard pinned = pos.blockers_for_king(us) & pos.pieces(us);
  bool validate = false;
#ifdef GRID
  if (pos.is_grid()) validate = true;
#endif
#ifdef RACE
  if (pos.is_race()) validate = true;
#endif
#ifdef TWOKINGS
  if (pos.is_two_kings()) validate = true;
#endif
#ifdef PLACEMENT
  if (pos.is_placement() && pos.count_in_hand<ALL_PIECES>(us)) validate = true;
#endif
#ifdef KNIGHTRELAY
  if (pos.is_knight_relay()) validate = pos.pieces(KNIGHT);
#endif
#ifdef RELAY
  if (pos.is_relay()) validate = pos.pieces(~us) ^ pos.pieces(~us, PAWN, KING);
#endif
  Square ksq;
#ifdef HORDE
  if (pos.is_horde() && pos.is_horde_color(pos.side_to_move()))
      ksq = SQ_NONE;
  else
#endif
  ksq = pos.square<KING>(us);
  ExtMove* cur = moveList;
  moveList = pos.checkers() ? generate<EVASIONS    >(pos, moveList)
                            : generate<NON_EVASIONS>(pos, moveList);
  while (cur != moveList)
      if ((validate
#ifdef ATOMIC
                  || (pos.is_atomic() && pos.capture(*cur))
#endif
#ifdef CRAZYHOUSE
                  || (pos.is_house() && type_of(*cur) == DROP)
#endif
                  || ((pinned & from_sq(*cur)) || from_sq(*cur) == ksq || type_of(*cur) == EN_PASSANT))
               && !pos.legal(*cur))
          *cur = (--moveList)->move;
      else
          ++cur;

  return moveList;
}

} // namespace Stockfish
