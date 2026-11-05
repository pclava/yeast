#include "log.h"
#include "fs.h"

void log_init(struct superblock *sb) {
    // not implemented
    (void)sb;
}

void log_write(struct bbuf *b) {
    // not implemented
    bwrite(b);
}

void begin_op(void) {
    return;
}

void end_op(void) {
    return;
}