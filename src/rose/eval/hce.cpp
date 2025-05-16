#include "rose/eval/hce.h"

#include <array>

#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::eval {

  // clang-format off
  static constexpr std::array<i16, 64> p_mg{{
      0,   0,   0,   0,   0,   0,   0,   0,
      18,  53,  42,  28,  38,  77,  103, 37,
      28,  58,  54,  50,  61,  54,  95,  44,
      27,  52,  57,  70,  73,  61,  60,  31,
      27,  63,  59,  77,  84,  66,  66,  27,
      33,  83,  61,  99,  99,  118, 34,  59,
      164, 151, 135, 186, 180, 102, -15, 30,
      0,   0,   0,   0,   0,   0,   0,   0,
  }};

  static constexpr std::array<i16, 64> p_eg{{
      0,   0,   0,   0,   0,   0,   0,   0,
      197, 197, 189, 186, 183, 179, 181, 164,
      189, 187, 170, 173, 170, 172, 178, 167,
      203, 195, 173, 165, 163, 165, 194, 177,
      242, 221, 184, 182, 160, 179, 208, 207,
      314, 298, 291, 252, 244, 209, 329, 266,
      392, 403, 376, 314, 297, 368, 432, 380,
      0,   0,   0,   0,   0,   0,   0,   0,
  }};

  static constexpr std::array<i16, 64> n_mg{{
      191, 233, 236, 230, 239, 259, 239, 221,
      214, 225, 250, 254, 255, 266, 250, 251,
      238, 259, 262, 274, 274, 273, 278, 236,
      253, 265, 275, 276, 284, 283, 284, 265,
      265, 272, 296, 306, 292, 326, 281, 297,
      276, 297, 302, 338, 354, 352, 303, 259,
      233, 268, 293, 318, 302, 354, 237, 249,
      134, 183, 168, 238, 307, 70,  159, 142,
  }};

  static constexpr std::array<i16, 64> n_eg{{
      379, 347, 379, 406, 392, 382, 359, 321,
      366, 410, 419, 425, 418, 395, 407, 372,
      393, 418, 431, 455, 466, 434, 423, 388,
      402, 447, 472, 474, 464, 456, 441, 411,
      412, 452, 474, 469, 479, 457, 450, 418,
      406, 434, 462, 457, 445, 444, 457, 408,
      397, 411, 431, 440, 424, 411, 383, 402,
      326, 425, 445, 418, 394, 453, 417, 343,
  }};

  static constexpr std::array<i16, 64> b_mg{{
      223, 274, 257, 259, 267, 246, 267, 239,
      271, 275, 279, 263, 271, 282, 290, 269,
      268, 281, 275, 280, 271, 285, 282, 277,
      270, 280, 286, 300, 303, 283, 279, 274,
      258, 279, 294, 319, 314, 301, 290, 276,
      273, 277, 284, 318, 332, 331, 294, 304,
      241, 279, 274, 274, 291, 282, 286, 265,
      235, 242, 230, 198, 208, 118, 212, 265,
  }};

  static constexpr std::array<i16, 64> b_eg{{
      417, 400, 383, 438, 419, 408, 418, 426,
      414, 420, 438, 451, 453, 435, 423, 419,
      432, 451, 481, 481, 488, 462, 447, 421,
      420, 475, 490, 495, 475, 481, 458, 421,
      450, 477, 481, 483, 483, 462, 468, 456,
      452, 491, 484, 478, 455, 483, 479, 426,
      452, 449, 475, 462, 471, 465, 461, 413,
      434, 460, 467, 472, 495, 494, 458, 454,
  }};

  static constexpr std::array<i16, 64> r_mg{{
      321, 328, 333, 339, 343, 325, 303, 321,
      304, 323, 311, 317, 331, 344, 347, 284,
      311, 308, 327, 323, 330, 327, 341, 321,
      317, 315, 326, 334, 345, 346, 343, 331,
      323, 333, 356, 359, 356, 354, 356, 340,
      338, 363, 368, 394, 423, 418, 394, 374,
      368, 376, 397, 423, 431, 424, 374, 370,
      416, 409, 427, 441, 421, 416, 424, 430,
  }};

  static constexpr std::array<i16, 64> r_eg{{
      747, 760, 771, 771, 762, 765, 767, 706,
      743, 747, 760, 755, 743, 755, 729, 723,
      741, 760, 755, 767, 752, 761, 744, 731,
      772, 784, 794, 784, 775, 775, 769, 746,
      796, 803, 802, 805, 798, 803, 790, 778,
      813, 808, 807, 803, 794, 794, 796, 787,
      806, 810, 809, 801, 798, 787, 803, 799,
      780, 787, 782, 786, 791, 792, 790, 777,
  }};

  static constexpr std::array<i16, 64> q_mg{{
      918, 921, 922, 930, 926, 902, 932, 948,
      918, 930, 937, 923, 930, 951, 938, 912,
      916, 933, 928, 921, 917, 929, 930, 926,
      926, 912, 922, 929, 931, 926, 937, 924,
      922, 916, 918, 932, 938, 944, 933, 940,
      917, 920, 930, 952, 972, 975, 956, 956,
      908, 905, 932, 907, 911, 950, 924, 935,
      862, 970, 959, 990, 975, 979, 914, 909,
  }};

  static constexpr std::array<i16, 64> q_eg{{
      1211, 1192, 1201, 1203, 1192, 1187, 1151, 1125,
      1163, 1238, 1224, 1245, 1236, 1193, 1181, 1200,
      1242, 1243, 1279, 1277, 1292, 1282, 1290, 1241,
      1248, 1294, 1314, 1343, 1339, 1335, 1293, 1285,
      1265, 1312, 1352, 1344, 1381, 1350, 1372, 1304,
      1293, 1301, 1324, 1353, 1355, 1378, 1345, 1330,
      1299, 1325, 1352, 1379, 1401, 1355, 1355, 1325,
      1356, 1277, 1322, 1331, 1329, 1320, 1356, 1353,
  }};

  static constexpr std::array<i16, 64> k_mg{{
      -73, 7,   -17, -92,  -23,  -59,  31,   25,
      11,  -23, -41, -94,  -92,  -54,  -10,  10,
      -37, -49, -98, -125, -157, -105, -66,  -74,
      -54, 7,   -97, -73,  -122, -132, -107, -133,
      -10, -58, -54, -125, -77,  -54,  -109, -154,
      -7,  24,  -46, -1,   -52,  -22,  -52,  -106,
      153, 61,  33,  29,   -22,  -64,  -64,  -73,
      438, 171, 67,  111,  92,   65,   153,  118,
  }};

  static constexpr std::array<i16, 64> k_eg{{
      -52,  -73, -58, -60, -102, -68, -89, -112,
      -68,  -43, -25, -8,  -17,  -24, -40, -68,
      -37,  -19, -4,  17,  24,   9,   -14, -39,
      -33,  -2,  24,  21,  31,   32,  13,  -21,
      -14,  20,  34,  43,  40,   47,  51,  23,
      -7,   17,  28,  27,  34,   36,  56,  34,
      -54,  0,   17,  13,  19,   54,  52,  25,
      -195, -44, 5,   -7,  21,   30,  -42, -52,
  }};
  // clang-format on

  static auto phase(const Position &position) -> i32 {
    // none, king, pawn, knight, none, bishop, rook, queen
    static constexpr std::array<i8, 8> piece_values{{0, 0, 0, 1, 0, 1, 2, 4}};
    i32 phase = 0;
    for (const PieceType white_ptype : position.pieceListType(Color::white).m)
      phase += piece_values[white_ptype.toIndex()];
    for (const PieceType black_ptype : position.pieceListType(Color::black).m)
      phase += piece_values[black_ptype.toIndex()];
    return std::min(phase, 24);
  }

  auto hce(const Position &position) -> i32 {
    const i32 mg_phase = phase(position);
    const i32 eg_phase = 24 - mg_phase;
    i32 score = 0;
    for (int i = 0; i < 16; i++) {
      const PieceType ptype = position.pieceListType(Color::white).m[i];
      const Square where = position.pieceListSq(Color::white).m[i];
      switch (ptype.raw) {
      case PieceType::none:
        break;
      case PieceType::k:
        score += k_mg[where.raw] * mg_phase + k_eg[where.raw] * eg_phase;
        break;
      case PieceType::q:
        score += q_mg[where.raw] * mg_phase + q_eg[where.raw] * eg_phase;
        break;
      case PieceType::r:
        score += r_mg[where.raw] * mg_phase + r_eg[where.raw] * eg_phase;
        break;
      case PieceType::b:
        score += b_mg[where.raw] * mg_phase + b_eg[where.raw] * eg_phase;
        break;
      case PieceType::n:
        score += n_mg[where.raw] * mg_phase + n_eg[where.raw] * eg_phase;
        break;
      case PieceType::p:
        score += p_mg[where.raw] * mg_phase + p_eg[where.raw] * eg_phase;
        break;
      }
    }
    for (int i = 0; i < 16; i++) {
      const PieceType ptype = position.pieceListType(Color::black).m[i];
      const Square where = position.pieceListSq(Color::black).m[i].flipRanks();
      switch (ptype.raw) {
      case PieceType::none:
        break;
      case PieceType::k:
        score -= k_mg[where.raw] * mg_phase + k_eg[where.raw] * eg_phase;
        break;
      case PieceType::q:
        score -= q_mg[where.raw] * mg_phase + q_eg[where.raw] * eg_phase;
        break;
      case PieceType::r:
        score -= r_mg[where.raw] * mg_phase + r_eg[where.raw] * eg_phase;
        break;
      case PieceType::b:
        score -= b_mg[where.raw] * mg_phase + b_eg[where.raw] * eg_phase;
        break;
      case PieceType::n:
        score -= n_mg[where.raw] * mg_phase + n_eg[where.raw] * eg_phase;
        break;
      case PieceType::p:
        score -= p_mg[where.raw] * mg_phase + p_eg[where.raw] * eg_phase;
        break;
      }
    }
    score /= 24;
    return position.activeColor() == Color::white ? score : -score;
  }

} // namespace rose::eval
