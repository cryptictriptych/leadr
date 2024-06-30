#pragma once

#ifdef MULTI_LEADR
// if user wants to have multiple leadr keys it's better to assume they will
// define their own keys to avoid conflicts with reserved keycodes. In this
// case the custom keycodes must be known before loading leadr_sequences.def.
// The workaround is to define custom keycodes in keymap.h instead of keymap.c,
// but that needs to be included here. Need a better way to handle this, but
// for now multiple leadr keys will require a keymap.h.
#include "keymap.h"
#endif
#include "quantum.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef LEADR_INPUT_MAXLEN
#define LEADR_INPUT_MAXLEN 6
#endif
#ifndef LEADR_OUTPUT_MAXLEN
#define LEADR_OUTPUT_MAXLEN 64
#endif

enum leadr_end_type {
    LT_NORM,  // leader sequence ended by reaching a unique entry
    LT_INVL,  // sequence aborted due to invalid sequence
    LT_ABRT,  // sequence aborted using LEADR_CANCEL_KEY
    LT_UNIQ,  // sequence completed early because remaining characters form a unique branch
    LT_FORC,  // multiple branches possible, but leader key triggered current match
    LT_CONT,  // multiple branches possible, but invalid continuation key forced current match 
};

enum leadr_record_state {
    RECORD_NONE,
    RECORD_INDEX,
    RECORD_INPUT,
    RECORD_OUTPUT,
};

extern uint16_t leadr_key;
extern uint16_t leadr_record_keycode;
bool leadr_active(void);
void init_leadr_sequence(void);
bool leadr_process(uint16_t, keyrecord_t*);
bool leadr_end(uint16_t, enum leadr_end_type);
enum leadr_record_state leadr_macro_status(void);

#define LEAD(out, ...)
#define LEADLBL(out, ...) ,out
#define LEADACT(out, ...) ,out
enum leadr_id {
    NOLEADRID
#include "leadr_sequences.def"
};
#undef LEAD
#undef LEADLBL
#undef LEADACT

typedef struct {
    enum leadr_id label;
    char output[LEADR_OUTPUT_MAXLEN]; 
    uint16_t input[LEADR_INPUT_MAXLEN];
} leadr_entry;

#define SET_LEADR_KEY(lkey) uint16_t leadr_key = lkey;
#define SET_LEADR_RECORD_KEY(lkey) uint16_t leadr_record_keycode = lkey;
