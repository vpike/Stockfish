#ifdef LOMONOSOV_TB
#include "lomonosov_probe.h"
#include "position.h"
#include "search.h"
#include "bitcount.h"
#include "movegen.h"
#include "lmtb.h"

inline int position_sign(int value) {
	if (value < 0)
		return -1;
	if (value > 0)
		return 1;
	return 0;
}

bool lomonosov_tbprobe(Position& pos, int ss_ply, int *value, bool ce_value, bool *from_dtm) {
	*value = VALUE_DRAW;
	int side; unsigned int psqW[KING_INDEX + 1]; unsigned int psqB[KING_INDEX + 1]; int piCount[10]; int sqEnP;
	pos.lomonosov_position(&side, psqW, psqB, piCount, &sqEnP);
	int eval = 0;
	char table_type = 0;
	int success = tb_probe_position_with_order(side, psqW, psqB, piCount, sqEnP, &eval, 0, &table_type);
	if (from_dtm)
		*from_dtm = false;
	int dtm = MAX_PLY;
	if (DTM_TYPE(table_type)) {
		if (from_dtm)
			*from_dtm = true;
		if (!DTZ50_TYPE(table_type) && eval == -1) // mate
			dtm = 0;
		else
			dtm = abs(eval);
	}
	if (success == 0) {
		int sign = position_sign(eval);
		*value = (ce_value ? (sign * (VALUE_MATE - dtm - ss_ply))
			: (sign * (dtm + ss_ply)));
		return true;
	}
	return false;
}

bool lomonosov_root_probe(Position& pos, bool *from_dtm) {
	int value;
	*from_dtm = false;

	if (!lomonosov_tbprobe(pos, 0, &value, false)) return false;

	StateInfo st;
	CheckInfo ci(pos);
	bool success = false;
	bool from_dtm_moves = true;

	// Probe each move.
	for (size_t i = 0; i < Search::RootMoves.size(); i++) {
		Move move = Search::RootMoves[i].pv[0];
		pos.do_move(move, st, ci, pos.gives_check(move, ci));
		int v = 0;
		bool from_dtm_cur = false;
		success = lomonosov_tbprobe(pos, 1, &v, false, &from_dtm_cur);
		pos.undo_move(move);
		if (!success) return false;
		from_dtm_moves &= from_dtm_cur;
		Search::RootMoves[i].score = (Value)v;
	}

	size_t j = 0;
	if (value > 0) {
		int best = 0;
		if (from_dtm_moves) {
			for (size_t i = 0; i < Search::RootMoves.size(); i++) {
				int v = Search::RootMoves[i].score;
				if (v < 0 && (v > best || best == 0))
					best = v;
			}
		}
		for (size_t i = 0; i < Search::RootMoves.size(); i++) {
			int v = Search::RootMoves[i].score;
			if (from_dtm_moves) {
				if (v == best) {
					Search::RootMoves[j++] = Search::RootMoves[i];
					Search::RootMoves[0].score = VALUE_MATE - value;
					*from_dtm = true;
					break;
				}
			} else if (v < 0) {
				Search::RootMoves[j++] = Search::RootMoves[i];
			}
		}
	} else if (value < 0) {
		if (from_dtm_moves) {
			int best = 0;
			for (size_t i = 0; i < Search::RootMoves.size(); i++) {
				int v = Search::RootMoves[i].score;
				if (v > best)
					best = v;
			}
			for (size_t i = 0; i < Search::RootMoves.size(); i++) {
				if (Search::RootMoves[i].score == best)
					Search::RootMoves[j++] = Search::RootMoves[i];
			}
		}
	} else {
		for (size_t i = 0; i < Search::RootMoves.size(); i++) {
			if (Search::RootMoves[i].score == 0)
				Search::RootMoves[j++] = Search::RootMoves[i];
		}
	}
	Search::RootMoves.resize(j, Search::RootMove(MOVE_NONE));

	return true;
}

#endif // LOMONOSOV_TB