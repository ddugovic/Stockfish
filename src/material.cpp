/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include <algorithm> // For std::min
#include <cassert>
#include <cstring>   // For std::memset

#include "material.h"
#include "thread.h"

using namespace std;

namespace {

  // Polynomial material imbalance parameters

  const int QuadraticOurs[VARIANT_NB][PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    {
    //            OUR PIECES
    // pair pawn knight bishop rook queen
    {1667                               }, // Bishop pair
    {  40,    0                         }, // Pawn
    {  32,  255,  -3                    }, // Knight      OUR PIECES
    {   0,  104,   4,    0              }, // Bishop
    { -26,   -2,  47,   105,  -149      }, // Rook
    {-185,   24, 122,   137,  -134,   0 }  // Queen
    },
#ifdef ANTI
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen king
      { -129                                    }, // Bishop pair
      { -205,   49                              }, // Pawn
      {  -81,  436,  -81                        }, // Knight      OUR PIECES
      {    0, -204, -328,    0                  }, // Bishop
      { -197, -436,  -12, -183,   92            }, // Rook
      {  197,   40,  133, -179,   93, -66       }, // Queen
      {    1,  -48,   98,   36,   82, 165, -168 }  // King
    },
#endif
#ifdef ATOMIC
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {  37,  -18                         }, // Pawn
      {  38,  261, -10                    }, // Knight      OUR PIECES
      {   0,   91,  -2,     0             }, // Bishop
      { -19,  -18,  28,    90,  -149      }, // Rook
      {-175,   18, 109,   149,  -124,   0 }  // Queen
    },
#endif
#ifdef CRAZYHOUSE
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {  40,    2                         }, // Pawn
      {  32,  255,  -3                    }, // Knight      OUR PIECES
      {   0,  104,   4,    0              }, // Bishop
      { -26,   -2,  47,   105,  -149      }, // Rook
      {-185,   24, 122,   137,  -134,   0 }  // Queen
    },
#endif
#ifdef HORDE
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen king
      {  13                                      }, // Bishop pair
      {  -2,    0                                }, // Pawn
      { -65,   66,   15                          }, // Knight      OUR PIECES
      {   0,   81,   -2,    0                    }, // Bishop
      {  26,   21,  -38,   80,   -70             }, // Rook
      {  24,  -27,   75,   32,     2,  -70       }, // Queen
      {   0,    0,    0,    0,     0,    0,    0 }  // King
    },
#endif
#ifdef KOTH
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {  40,    2                         }, // Pawn
      {  32,  255,  -3                    }, // Knight      OUR PIECES
      {   0,  104,   4,    0              }, // Bishop
      { -26,   -2,  47,   105,  -149      }, // Rook
      {-185,   24, 122,   137,  -134,   0 }  // Queen
    },
#endif
#ifdef LOSERS
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1634                               }, // Bishop pair
      {  24,  156                         }, // Pawn
      {  90,  243, 133                    }, // Knight      OUR PIECES
      {   0,  120,  66,     0             }, // Bishop
      {  11,   -2,  41,    15,  -166      }, // Rook
      {-251,  258,  86,   141,  -205,  43 }  // Queen
    },
#endif
#ifdef RACE
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {   0,    0                         }, // Pawn
      {  32,    0,  -3                    }, // Knight      OUR PIECES
      {   0,    0,   4,    0              }, // Bishop
      { -26,    0,  47,   105,  -149      }, // Rook
      {-185,    0, 122,   137,  -134,   0 }  // Queen
    },
#endif
#ifdef RELAY
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {  40,    2                         }, // Pawn
      {  32,  255,  -3                    }, // Knight      OUR PIECES
      {   0,  104,   4,    0              }, // Bishop
      { -26,   -2,  47,   105,  -149      }, // Rook
      {-185,   24, 122,   137,  -134,   0 }  // Queen
    },
#endif
#ifdef THREECHECK
    {
      //            OUR PIECES
      // pair pawn knight bishop rook queen
      {1667                               }, // Bishop pair
      {  40,    2                         }, // Pawn
      {  32,  255,  -3                    }, // Knight      OUR PIECES
      {   0,  104,   4,    0              }, // Bishop
      { -26,   -2,  47,   105,  -149      }, // Rook
      {-185,   24, 122,   137,  -134,   0 }  // Queen
    },
#endif
  };
#ifdef CRAZYHOUSE
  const int QuadraticOursInHand[PIECE_TYPE_NB][PIECE_TYPE_NB] = {
      //            OUR PIECES
      //empty pawn knight bishop rook queen
      { -88                               }, // Empty hand
      {   0,  -44                         }, // Pawn
      {   0,   33,  21                    }, // Knight      OUR PIECES
      {   0,  -52, -16,    -6             }, // Bishop
      {   0,   14,  44,   -54,    19      }, // Rook
      {   0,   -5,   5,   -19,     2, -29 }  // Queen
  };
#endif

  const int QuadraticTheirs[VARIANT_NB][PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    {
    //           THEIR PIECES
    // pair pawn knight bishop rook queen
    {   0                               }, // Bishop pair
    {  36,    0                         }, // Pawn
    {   9,   63,   0                    }, // Knight      OUR PIECES
    {  59,   65,  42,     0             }, // Bishop
    {  46,   39,  24,   -24,    0       }, // Rook
    { 101,  100, -37,   141,  268,    0 }  // Queen
    },
#ifdef ANTI
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen king
      {    0                                   }, // Bishop pair
      {   55,    0                             }, // Pawn
      {   23,   27,    0                       }, // Knight      OUR PIECES
      {  -37, -248,  -18,    0                 }, // Bishop
      { -109, -628, -145,  102,   0            }, // Rook
      { -156, -133,  134,   78,  48,    0      }, // Queen
      {   22,  155,   84,   49, -49, -104,   0 }  // King
    },
#endif
#ifdef ATOMIC
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {  57,    0                         }, // Pawn
      {  16,   47,   0                    }, // Knight      OUR PIECES
      {  65,   62,  29,     0             }, // Bishop
      {  31,   54,  17,   -18,    0       }, // Rook
      { 105,   97, -34,   151,  278,    0 }  // Queen
    },
#endif
#ifdef CRAZYHOUSE
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {  36,    0                         }, // Pawn
      {   9,   63,   0                    }, // Knight      OUR PIECES
      {  59,   65,  42,     0             }, // Bishop
      {  46,   39,  24,   -24,    0       }, // Rook
      { 101,  100, -37,   141,  268,    0 }  // Queen
    },
#endif
#ifdef HORDE
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen king
      { 0                                       }, // Bishop pair
      { 0,     0                                }, // Pawn
      { 0,     0,     0                         }, // Knight      OUR PIECES
      { 0,     0,     0,     0                  }, // Bishop
      { 0,     0,     0,     0,     0           }, // Rook
      { 0,     0,     0,     0,     0,    0     }, // Queen
      { 0,  -557,  -711,   -86,  -386, -655,  0 }  // King
    },
#endif
#ifdef KOTH
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {  36,    0                         }, // Pawn
      {   9,   63,   0                    }, // Knight      OUR PIECES
      {  59,   65,  42,     0             }, // Bishop
      {  46,   39,  24,   -24,    0       }, // Rook
      { 101,  100, -37,   141,  268,    0 }  // Queen
    },
#endif
#ifdef LOSERS
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                                }, // Bishop pair
      {-132,    0                          }, // Pawn
      {  -5,  185,    0                    }, // Knight      OUR PIECES
      {  59,  440, -106,     0             }, // Bishop
      { 277,   30,    5,    27,    0       }, // Rook
      { 217,  357,    5,    51,  254,    0 }  // Queen
    },
#endif
#ifdef RACE
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {   0,    0                         }, // Pawn
      {   9,    0,   0                    }, // Knight      OUR PIECES
      {  59,    0,  42,     0             }, // Bishop
      {  46,    0,  24,   -24,    0       }, // Rook
      { 101,    0, -37,   141,  268,    0 }  // Queen
    },
#endif
#ifdef RELAY
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {  36,    0                         }, // Pawn
      {   9,   63,   0                    }, // Knight      OUR PIECES
      {  59,   65,  42,     0             }, // Bishop
      {  46,   39,  24,   -24,    0       }, // Rook
      { 101,  100, -37,   141,  268,    0 }  // Queen
    },
#endif
#ifdef THREECHECK
    {
      //           THEIR PIECES
      // pair pawn knight bishop rook queen
      {   0                               }, // Bishop pair
      {  36,    0                         }, // Pawn
      {   9,   63,   0                    }, // Knight      OUR PIECES
      {  59,   65,  42,     0             }, // Bishop
      {  46,   39,  24,   -24,    0       }, // Rook
      { 101,  100, -37,   141,  268,    0 }  // Queen
    },
#endif
  };

  // PawnsSet[count] contains a bonus/malus indexed by number of pawns
  const int PawnsSet[FILE_NB + 1] = {
     24, -32, 107, -51, 117, -9, -126, -21, 31
  };

  // Endgame evaluation and scaling functions are accessed directly and not through
  // the function maps because they correspond to more than one material hash key.
  Endgame<CHESS_VARIANT, KXK>    EvaluateKXK[] = { Endgame<CHESS_VARIANT, KXK>(WHITE),    Endgame<CHESS_VARIANT, KXK>(BLACK) };
#ifdef ATOMIC
  Endgame<ATOMIC_VARIANT, KXK> EvaluateAtomicKXK[] = { Endgame<ATOMIC_VARIANT, KXK>(WHITE), Endgame<ATOMIC_VARIANT, KXK>(BLACK) };
#endif

  Endgame<CHESS_VARIANT, KBPsK>  ScaleKBPsK[]  = { Endgame<CHESS_VARIANT, KBPsK>(WHITE),  Endgame<CHESS_VARIANT, KBPsK>(BLACK) };
  Endgame<CHESS_VARIANT, KQKRPs> ScaleKQKRPs[] = { Endgame<CHESS_VARIANT, KQKRPs>(WHITE), Endgame<CHESS_VARIANT, KQKRPs>(BLACK) };
  Endgame<CHESS_VARIANT, KPsK>   ScaleKPsK[]   = { Endgame<CHESS_VARIANT, KPsK>(WHITE),   Endgame<CHESS_VARIANT, KPsK>(BLACK) };
  Endgame<CHESS_VARIANT, KPKP>   ScaleKPKP[]   = { Endgame<CHESS_VARIANT, KPKP>(WHITE),   Endgame<CHESS_VARIANT, KPKP>(BLACK) };

  // Helper used to detect a given material distribution
  bool is_KXK(const Position& pos, Color us) {
    return  !more_than_one(pos.pieces(~us))
          && pos.non_pawn_material(us) >= RookValueMg;
  }

#ifdef ATOMIC
  bool is_KXK_atomic(const Position& pos, Color us) {
    return  !more_than_one(pos.pieces(~us))
          && pos.non_pawn_material(us) >= RookValueMg + KnightValueMg;
  }
#endif

  bool is_KBPsKs(const Position& pos, Color us) {
    return   pos.non_pawn_material(us) == BishopValueMg
          && pos.count<BISHOP>(us) == 1
          && pos.count<PAWN  >(us) >= 1;
  }

  bool is_KQKRPs(const Position& pos, Color us) {
    return  !pos.count<PAWN>(us)
          && pos.non_pawn_material(us) == QueenValueMg
          && pos.count<QUEEN>(us)  == 1
          && pos.count<ROOK>(~us) == 1
          && pos.count<PAWN>(~us) >= 1;
  }

  /// imbalance() calculates the imbalance by comparing the piece count of each
  /// piece type for both colors.
  template<Color Us>
#ifdef CRAZYHOUSE
  int imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB],
                const int pieceCountInHand[][PIECE_TYPE_NB]) {
#else
  int imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB]) {
#endif

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    int bonus = PawnsSet[std::min(pieceCount[Us][PAWN], (int)FILE_NB)];

    // Second-degree polynomial material imbalance by Tord Romstad
    PieceType pt_max =
#ifdef ANTI
                      pos.is_anti() ? KING :
#endif
#ifdef HORDE
                      pos.is_horde() ? KING :
#endif
                      QUEEN;

  const int *pieceCountUs = pieceCount[Us];
  const int *pieceCountThem = pieceCount[Them];
#ifdef CRAZYHOUSE
  if (pos.is_house()) {
    int pieceTotalUs[PIECE_TYPE_NB];
    int pieceTotalThem[PIECE_TYPE_NB];

    for (int pt1 = NO_PIECE_TYPE; pt1 <= pt_max; ++pt1) {
      pieceTotalUs[pt1] = pieceCountInHand[Us][pt1] + pieceCountUs[pt1];
      pieceTotalThem[pt1] = pieceCountInHand[Them][pt1] + pieceCountThem[pt1];
    }

    pieceCountUs = pieceTotalUs;
    pieceCountThem = pieceTotalThem;

    for (int pt1 = NO_PIECE_TYPE; pt1 <= pt_max; ++pt1) {
        if (!pieceCountInHand[Us][pt1])
            continue;

        int v = 0;

        for (int pt2 = NO_PIECE_TYPE; pt2 <= pt1; ++pt2)
            v += QuadraticOursInHand[pt1][pt2] * pieceCountInHand[Us][pt2];

        bonus += pieceCountInHand[Us][pt1] * v;
    }
  }
#endif

    const int variant = pos.variant();

    for (int pt1 = NO_PIECE_TYPE; pt1 <= pt_max; ++pt1)
    {
        if (!pieceCountUs[pt1])
            continue;

        int v = 0;

        for (int pt2 = NO_PIECE_TYPE; pt2 <= pt1; ++pt2)
            v +=  QuadraticOurs[variant][pt1][pt2] * pieceCountUs[pt2]
                + QuadraticTheirs[variant][pt1][pt2] * pieceCountThem[pt2];

        bonus += pieceCountUs[pt1] * v;
    }

    return bonus;
  }
} // namespace

namespace Material {

/// Material::probe() looks up the current position's material configuration in
/// the material hash table. It returns a pointer to the Entry if the position
/// is found. Otherwise a new Entry is computed and stored there, so we don't
/// have to recompute all when the same material configuration occurs again.

Entry* probe(const Position& pos) {

  Key key = pos.material_key();
  Entry* e = pos.this_thread()->materialTable[key];

  if (e->key == key)
      return e;

  std::memset(e, 0, sizeof(Entry));
  e->key = key;
  e->factor[WHITE] = e->factor[BLACK] = (uint8_t)SCALE_FACTOR_NORMAL;
  e->gamePhase = pos.game_phase();

  // Let's look if we have a specialized evaluation function for this particular
  // material configuration. Firstly we look for a fixed configuration one, then
  // for a generic one if the previous search failed.
  if ((e->evaluationFunction = pos.this_thread()->endgames.probe<Value>(key)) != nullptr)
      return e;

  if (pos.variant() == CHESS_VARIANT)
  {
  for (Color c = WHITE; c <= BLACK; ++c)
      if (is_KXK(pos, c))
      {
          e->evaluationFunction = &EvaluateKXK[c];
          return e;
      }
  }
#ifdef ATOMIC
  else if (pos.is_atomic())
  {
      for (Color c = WHITE; c <= BLACK; ++c)
          if (is_KXK_atomic(pos, c))
          {
              e->evaluationFunction = &EvaluateAtomicKXK[c];
              return e;
          }
  }
#endif

  // OK, we didn't find any special evaluation function for the current material
  // configuration. Is there a suitable specialized scaling function?
  EndgameBase<ScaleFactor>* sf;

  if ((sf = pos.this_thread()->endgames.probe<ScaleFactor>(key)) != nullptr)
  {
      e->scalingFunction[sf->strong_side()] = sf; // Only strong color assigned
      return e;
  }

  if (pos.variant() == CHESS_VARIANT)
  {
  // We didn't find any specialized scaling function, so fall back on generic
  // ones that refer to more than one material distribution. Note that in this
  // case we don't return after setting the function.
  for (Color c = WHITE; c <= BLACK; ++c)
  {
    if (is_KBPsKs(pos, c))
        e->scalingFunction[c] = &ScaleKBPsK[c];

    else if (is_KQKRPs(pos, c))
        e->scalingFunction[c] = &ScaleKQKRPs[c];
  }

  Value npm_w = pos.non_pawn_material(WHITE);
  Value npm_b = pos.non_pawn_material(BLACK);

  if (npm_w + npm_b == VALUE_ZERO && pos.pieces(PAWN)) // Only pawns on the board
  {
      if (!pos.count<PAWN>(BLACK))
      {
          assert(pos.variant() != CHESS_VARIANT || pos.count<PAWN>(WHITE) >= 2);

          e->scalingFunction[WHITE] = &ScaleKPsK[WHITE];
      }
      else if (!pos.count<PAWN>(WHITE))
      {
          assert(pos.variant() != CHESS_VARIANT || pos.count<PAWN>(BLACK) >= 2);

          e->scalingFunction[BLACK] = &ScaleKPsK[BLACK];
      }
      else if (pos.count<PAWN>(WHITE) == 1 && pos.count<PAWN>(BLACK) == 1)
      {
          // This is a special case because we set scaling functions
          // for both colors instead of only one.
          e->scalingFunction[WHITE] = &ScaleKPKP[WHITE];
          e->scalingFunction[BLACK] = &ScaleKPKP[BLACK];
      }
  }

  // Zero or just one pawn makes it difficult to win, even with a small material
  // advantage. This catches some trivial draws like KK, KBK and KNK and gives a
  // drawish scale factor for cases such as KRKBP and KmmKm (except for KBBKN).
  if (!pos.count<PAWN>(WHITE) && npm_w - npm_b <= BishopValueMg)
      e->factor[WHITE] = uint8_t(npm_w <  RookValueMg   ? SCALE_FACTOR_DRAW :
                                 npm_b <= BishopValueMg ? 4 : 14);

  if (!pos.count<PAWN>(BLACK) && npm_b - npm_w <= BishopValueMg)
      e->factor[BLACK] = uint8_t(npm_b <  RookValueMg   ? SCALE_FACTOR_DRAW :
                                 npm_w <= BishopValueMg ? 4 : 14);

  if (pos.count<PAWN>(WHITE) == 1 && npm_w - npm_b <= BishopValueMg)
      e->factor[WHITE] = (uint8_t) SCALE_FACTOR_ONEPAWN;

  if (pos.count<PAWN>(BLACK) == 1 && npm_b - npm_w <= BishopValueMg)
      e->factor[BLACK] = (uint8_t) SCALE_FACTOR_ONEPAWN;
  }
#ifdef ATOMIC
  else if (pos.is_atomic())
  {
      Value npm_w = pos.non_pawn_material(WHITE);
      Value npm_b = pos.non_pawn_material(BLACK);
      if (!pos.pieces(PAWN) && npm_w + npm_b <= RookValueMg)
          e->factor[WHITE] = (uint8_t) SCALE_FACTOR_DRAW;
  }
#endif

  // Evaluate the material imbalance. We use PIECE_TYPE_NONE as a place holder
  // for the bishop pair "extended piece", which allows us to be more flexible
  // in defining bishop pair bonuses.
  const int PieceCount[COLOR_NB][PIECE_TYPE_NB] = {
  { pos.count<BISHOP>(WHITE) > 1, pos.count<PAWN>(WHITE), pos.count<KNIGHT>(WHITE),
    pos.count<BISHOP>(WHITE)    , pos.count<ROOK>(WHITE), pos.count<QUEEN >(WHITE), pos.count<KING>(WHITE) },
  { pos.count<BISHOP>(BLACK) > 1, pos.count<PAWN>(BLACK), pos.count<KNIGHT>(BLACK),
    pos.count<BISHOP>(BLACK)    , pos.count<ROOK>(BLACK), pos.count<QUEEN >(BLACK), pos.count<KING>(BLACK) } };
#ifdef CRAZYHOUSE
  const int PieceCountInHand[COLOR_NB][PIECE_TYPE_NB] = {
  { pos.count_in_hand<ALL_PIECES>(WHITE) == 0, pos.count_in_hand<PAWN>(WHITE), pos.count_in_hand<KNIGHT>(WHITE),
    pos.count_in_hand<BISHOP>(WHITE)         , pos.count_in_hand<ROOK>(WHITE), pos.count_in_hand<QUEEN >(WHITE), pos.count_in_hand<KING>(WHITE) },
  { pos.count_in_hand<ALL_PIECES>(BLACK) == 0, pos.count_in_hand<PAWN>(BLACK), pos.count_in_hand<KNIGHT>(BLACK),
    pos.count_in_hand<BISHOP>(BLACK)         , pos.count_in_hand<ROOK>(BLACK), pos.count_in_hand<QUEEN >(BLACK), pos.count_in_hand<KING>(BLACK) } };

  e->value = int16_t((imbalance<WHITE>(pos, PieceCount, PieceCountInHand) - imbalance<BLACK>(pos, PieceCount, PieceCountInHand)) / 16);
#else
  e->value = int16_t((imbalance<WHITE>(pos, PieceCount) - imbalance<BLACK>(pos, PieceCount)) / 16);
#endif
  return e;
}

} // namespace Material
