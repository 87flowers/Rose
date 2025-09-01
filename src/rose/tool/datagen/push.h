#include <vector>

#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose::tool::datagen {

  inline auto pushBytes(std::vector<u8> &output, const void *ptr_void, usize size) -> void {
    const char *ptr = reinterpret_cast<const char *>(ptr_void);
    for (usize i = 0; i < size; i++)
      output.push_back(static_cast<u8>(*ptr++));
  }

  template <typename T> inline auto push(std::vector<u8> &output, T value) -> void { pushBytes(output, &value, sizeof(T)); }

  inline auto pushString(std::vector<u8> &output, std::string_view str) -> void {
    push<u16>(output, str.size());
    pushBytes(output, str.data(), str.size());
  }

  inline auto pushMarlinformatPosition(std::vector<u8> &output, const Position &position) -> size_t {
    const Byteboard board = position.board();
    const RookInfo rook_info = position.rookInfo();

    push<u64>(output, board.getOccupiedBitboard());

    std::array<u8, 32> pieces{};
    for (u64 bb = board.getOccupiedBitboard(), i = 0; bb != 0; bb &= bb - 1, i++) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bb))};
      const Place p = board.read(sq);

      pieces[i] = [&] {
        switch (p.ptype().raw) {
        case PieceType::p:
          return 0;
        case PieceType::n:
          return 1;
        case PieceType::b:
          return 2;
        case PieceType::r:
          return rook_info.has(sq) ? 6 : 3;
        case PieceType::q:
          return 4;
        case PieceType::k:
          return 5;
        default:
          rose_assert(false);
          return 0;
        }
      }();

      if (p.color() == Color::black)
        pieces[i] |= 8;
    }
    for (int i = 0; i < 16; i++) {
      push<u8>(output, pieces[i * 2] | (pieces[i * 2 + 1] << 4));
    }

    push<u8>(output, position.activeColor().toMsb8() | (position.enpassant().isValid() ? position.enpassant().raw : 64));
    push<u8>(output, position.fiftyMoveClock());
    push<u16>(output, position.fullMoveCounter());
    push<u16>(output, 0); // Score
    const size_t game_result_index = output.size();
    push<u8>(output, 0); // Game result
    push<u8>(output, 0); // Padding

    return game_result_index;
  }

  inline auto pushViriformatMove(std::vector<u8> &output, Move move) {
    u16 result = 0;
    result |= static_cast<u16>(move.from().raw);
    result |= static_cast<u16>(move.to().raw) << 6;
    if (move.promo()) {
      result |= [&] {
        switch (move.ptype().raw) {
        case PieceType::n:
          return 0 << 12;
        case PieceType::b:
          return 1 << 12;
        case PieceType::r:
          return 2 << 12;
        case PieceType::q:
          return 3 << 12;
        default:
          rose_assert(false);
          return 0;
        }
      }();
    }
    if (move.enpassant())
      result |= 1 << 14;
    if (move.castle())
      result |= 2 << 14;
    if (move.promo())
      result |= 3 << 14;
    push<u16>(output, result);
  }

} // namespace rose::tool::datagen
