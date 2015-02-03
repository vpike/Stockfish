#ifdef LOMONOSOV_TB
#include "position.h"

bool lomonosov_tbprobe(Position& pos, int ss_ply, int *value, bool ce_value, bool *from_dtm = 0);
bool lomonosov_root_probe(Position& pos, bool *from_dtm = 0);
#endif