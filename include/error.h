#pragma once
#include <3ds.h>

static inline void sendError(char *error, int code) {
    // Seems like it doesn't like error message;
    errorConf err;
    errorInit(&err, ERROR_TEXT, CFG_LANGUAGE_EN);
    errorText(&err, error);
    errorCode(&err, code);
    errorDisp(&err);
}
