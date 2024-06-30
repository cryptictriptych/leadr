#include "leadr.h"
#include "quantum.h"
#include "print.h"

// listing some defines here for quick reference
// #define DOUBLE_LEADR_REPEAT_LAST 
// #define LEADR_AUTOCOMPLETE_UNIQ
// #define MULTI_LEADR
// #define MALLOC_LAST_VALID
// #define NUM_MACROS 4
// #define LEADR_MACRO_MAXLEN 128
// #define LEADR_CANCEL_KEY KC_ESC
// #define LEADR_INPUT_MAXLEN 6
// #define LEADR_OUTPUT_MAXLEN 64

#ifndef LEADR_KEY
#define LEADR_KEY leadr_key
#endif

#ifndef NUM_MACROS
#define NUM_MACROS 4
#endif

#ifndef LEADR_MACRO_MAXLEN
#define LEADR_MACRO_MAXLEN 128
#endif

#define LEADACT(lbl, ...) {.label=lbl, .output="", .input={__VA_ARGS__}},
#define LEADLBL(lbl, out, ...) {.label=lbl, .output=out, .input={__VA_ARGS__}},
#define LEAD(out, ...) {.label=NOLEADRID, .output=out, .input={__VA_ARGS__}},
leadr_entry leadr_table[] = {
#if NUM_MACROS > 0
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 1
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 2
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 3
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 4
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 5
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 6
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 7
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 8
    {NOLEADRID, "", {}},
#endif
#if NUM_MACROS > 9
    {NOLEADRID, "", {}},
#endif
#include "leadr_sequences.def"
};
#undef LEAD
#undef LEADLBL
#undef LEADACT

keyrecord_t macro_table[NUM_MACROS][LEADR_MACRO_MAXLEN];
uint8_t macro_lengths[NUM_MACROS];
bool _leadr_active = false;
uint16_t leadr_cnt = 0;
uint16_t leadr_candidate_cnt = 0;
const uint16_t total_leadr_num = ARRAY_SIZE(leadr_table);
uint16_t leadr_candidates[ARRAY_SIZE(leadr_table)];
uint16_t current_leadr;
uint16_t leadr_macro_in[LEADR_INPUT_MAXLEN];
char leadr_macro_out[LEADR_OUTPUT_MAXLEN];
uint8_t record_index = 0;
uint16_t leadr_record_keycode;
enum leadr_record_state record_state = RECORD_NONE;

#ifdef MULTI_LEADR
  uint8_t num_independent_leadrs;
  uint16_t distinct_leadrs[ARRAY_SIZE(leadr_table)];
# ifdef MALLOC_LAST_VALID
    // this will cut down on size, but could lead to issues if memory is
    // extremely tight
    uint16_t *last_valid_sequences;
# else
    uint16_t last_valid_sequences[ARRAY_SIZE(leadr_table)];
# endif
  bool leadr_uninitialized = true;
  uint8_t current_leadr_idx = 0;
#else
  uint16_t last_valid_sequence = 0;
#endif

enum leadr_record_state leadr_macro_status(void) {
    return record_state;
}

__attribute__((weak)) void leadr_start_user(uint16_t keycode) {}
__attribute__((weak)) void leadr_end_user(uint16_t keycode, enum leadr_end_type term_type) {}
__attribute__((weak)) void leadr_record_user(uint16_t keycode, keyrecord_t *record, enum leadr_record_state rec_state, bool success) {}
__attribute__((weak)) void leadr_process_user(enum leadr_id leadr_entry) {}

bool leadr_active(void) {
    return _leadr_active;
}

bool is_leadr(uint16_t keycode) {
#ifdef MULTI_LEADR
    for (uint8_t k=0; k<num_independent_leadrs; k++) {
        if (keycode == distinct_leadrs[k]) {
            current_leadr = keycode;
            current_leadr_idx = k;
            leadr_cnt = 1;
            return true;
        }
    }
    return false;
#else
    current_leadr = LEADR_KEY;
    return keycode == LEADR_KEY;
#endif
}

#ifdef MULTI_LEADR
//  Walk the leadr table to determine leadrs from the first key in each
//  sequence. Only used when multiple leadrs option is enabled.
void get_unique_leadrs(void) {
    uint16_t ll;
    num_independent_leadrs = 0;
    distinct_leadrs[num_independent_leadrs++] = leadr_table[0].input[0];
    for (int i=1; i<total_leadr_num; i++) {
        ll = leadr_table[i].input[0];
        if (is_leadr(ll)) {
            continue;
        }
        distinct_leadrs[num_independent_leadrs++] = ll;
    }
#ifdef MALLOC_LAST_VALID
    last_valid_sequences = malloc(sizeof(uint16_t)*num_independent_leadrs);
#endif
}
#endif

void init_leadr_sequence(void){
    uint16_t i;
#ifdef MULTI_LEADR
    if (leadr_uninitialized) {
        get_unique_leadrs();
        leadr_uninitialized = false;
    }
#endif
    leadr_cnt = 0;
    _leadr_active = true;
    leadr_candidate_cnt = total_leadr_num;
    for (i=0; i<total_leadr_num; i++) {
        leadr_candidates[i] = i;
    }
    leadr_start_user(current_leadr);
}

bool leadr_end(uint16_t keycode, enum leadr_end_type term_type) {
    _leadr_active = false;
    leadr_end_user(keycode, term_type);
    switch (term_type) {
        case LT_NORM...LT_FORC:
            return true;
        case LT_CONT:
            return false;
    }
    return true;
}

bool is_modifier(uint16_t keycode) {
    // return ((keycode & 0xF0FF) >= 0x00E0 && keycode < 0x56F0);
    return ((keycode & 0xF0FF) >= 0x00E0 && keycode < 0x56F0);
}
    
// return: "is captured as part of leadr sequence" (otherwise pass forward for regular handling)
bool record_leadr(uint16_t keycode, keyrecord_t *record) {
    static uint8_t rind = 0;
    if (record_state == RECORD_NONE) {
        // dprint("starting record\n");
        record_state = RECORD_INDEX;
        memset(leadr_macro_in, 0, LEADR_INPUT_MAXLEN*sizeof(uint16_t));
        memset(leadr_macro_out, 0, LEADR_OUTPUT_MAXLEN*sizeof(char));
        rind = 0;
#ifdef MULTI_LEADR
        leadr_macro_in[rind++] = current_leadr;
#endif
        leadr_record_user(keycode, record, record_state, true);
        return true;
    } else if (record_state == RECORD_INDEX) {
        if (record->event.pressed) {
            // dprint("getting recordindex\n");
            if (is_modifier(keycode)) {
                return false;
            }
            if (keycode == leadr_record_keycode) {
                goto abort;
            }
            switch (keycode) {
#if NUM_MACROS > 0
                case KC_0: record_index = 0; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 1
                case KC_1: record_index = 1; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 2
                case KC_2: record_index = 2; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 3
                case KC_3: record_index = 3; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 4
                case KC_4: record_index = 4; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 5
                case KC_5: record_index = 5; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 6
                case KC_6: record_index = 6; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 7
                case KC_7: record_index = 7; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 8
                case KC_8: record_index = 8; record_state = RECORD_INPUT; break;
#endif
#if NUM_MACROS > 9
                case KC_9: record_index = 9; record_state = RECORD_INPUT; break;
#endif
                default: goto abort;
            }
            // dprintf("recordindex %d\n", record_index);
            record_state = RECORD_INPUT;
            leadr_record_user(keycode, record, record_state, true);
            return true;
        }
        return false;
    } else if (record_state == RECORD_INPUT) {
        if (record->event.pressed) {
            // dprint("in inputrec\n");
            if (is_modifier(keycode)) {
                return false;
            }
            if (keycode == leadr_record_keycode) {
                goto abort;
            }
            if (keycode == current_leadr) {
                record_state = RECORD_OUTPUT;
                // dprintf("captured input sequence length: %d\n", rind);
                rind = 0;
                leadr_record_user(keycode, record, record_state, true);
                return true;
            } else {
                if (rind < LEADR_INPUT_MAXLEN) {
                    leadr_macro_in[rind++] = (uint16_t)keycode;
                    leadr_record_user(keycode, record, record_state, true);
                    return true;
                } else {
                    goto abort;
                }
            }
            // dprintf("inputkey %d\n", keycode);
        }
        return false;
    } else if (record_state == RECORD_OUTPUT) {
        if (keycode == leadr_record_keycode) {
            if (record->event.pressed) {
                goto abort;
            }
            return true;
        }
        if (keycode == current_leadr) {
            if (record->event.pressed) {
                memcpy((void*)&(leadr_table[record_index].input[0]), 
                       (void*)&leadr_macro_in[0], 
                       LEADR_INPUT_MAXLEN*sizeof(uint16_t));
                macro_lengths[record_index] = rind;
                record_state = RECORD_NONE;
                rind = 0;
                _leadr_active = false;
                leadr_record_user(keycode, record, record_state, true);
            }
            return true;
        }
        macro_table[record_index][rind++] = *record;
        leadr_record_user(keycode, record, record_state, true);
    }
    return false;
abort:
    // dprint("abort\n");
    _leadr_active = false;
    record_state = RECORD_NONE; 
    leadr_record_user(keycode, record, record_state, false);
    return false;
}

void send_sequence(uint16_t index) {
#ifdef MULTI_LEADR
    last_valid_sequences[current_leadr_idx] = index;
#else
    last_valid_sequence = index;
#endif
    uint16_t mods = get_mods(); 
    clear_oneshot_mods();
    clear_mods();
    _leadr_active = false;
    if (index < NUM_MACROS && macro_lengths[index] > 0) {
        for (int i=0; i<macro_lengths[index]; i++) {
            process_record(&(macro_table[index][i]));
        }
    } else {
        SEND_STRING(leadr_table[index].output);
    }
    set_mods(mods);
    leadr_process_user(leadr_table[index].label);
}

void send_last_sequence(void) {
#ifdef MULTI_LEADR
    send_sequence(last_valid_sequences[current_leadr_idx]);
#else
    send_sequence(last_valid_sequence);
#endif
}

bool leadr_process(uint16_t keycode, keyrecord_t *record) {
    uint16_t i, j = 0, terminal_candidate_index = 0;
    bool terminal_candidate_exists = false;
    if (!_leadr_active) {
#ifdef MULTI_LEADR
        if (leadr_uninitialized) {
            // multiple leadrs are only defined in LEAD() calls from the .def
            // file in order to simplify the API. Sift through unique leadr
            // keys, on the first call to amortize overhead up front instead of
            // inspecting the table each time (or complicating the api).
            get_unique_leadrs();
            leadr_uninitialized = false;
        }
#endif
        if (record->event.pressed) {
            if (is_leadr(keycode)) {
                init_leadr_sequence();
#ifndef MULTI_LEADR
                // only return if a single leadr is used, for multiple leadrs
                // we need continue on to start filtering out valid sequences
                return true;
#endif
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    if (record_state != RECORD_NONE || keycode == leadr_record_keycode) {
        // dprint("going to record \n");
        return record_leadr(keycode, record);
    }
    if (record->event.pressed) {
#if NUM_MACROS > 0
        if ((keycode & 0xF0FF) >= 0x00E0 && keycode < 0x56F0) {
            // dprintf("keycode %d\n",keycode);
            return false;
        }
#endif
#ifdef LEADR_CANCEL_KEY
        if (keycode == LEADR_CANCEL_KEY) {
            return leadr_end(keycode, LT_ABRT);
        }
#endif
        if (keycode == current_leadr 
#ifdef MULTI_LEADR
                // multiple leadrs are treated as part of the sequence to simplify
                // some of the logic, but we need to prevent cancellation/pushing
                // of a sequence output when the leadr itself is encountered in
                // the sequence.
                && leadr_cnt > 0 
#endif
           ) {
#ifdef DOUBLE_LEADR_REPEAT_LAST 
            if (leadr_cnt == 
# ifdef MULTI_LEADR
                    1 
# else
                    0
# endif
               ){
                send_last_sequence();
                return leadr_end(keycode, LT_FORC);
            }
#endif
            for (i=0; i<leadr_candidate_cnt; i++) {
                if (leadr_table[leadr_candidates[i]].input[leadr_cnt] == 0) {
                    send_sequence(leadr_candidates[i]);
                    return leadr_end(keycode, LT_FORC);
                }
            }
            return leadr_end(keycode, LT_INVL);
        }
        for (i=0; i<leadr_candidate_cnt; i++) {
            if (leadr_table[leadr_candidates[i]].input[leadr_cnt] == keycode) {
                leadr_candidates[j++] = leadr_candidates[i];
            } else if (leadr_table[leadr_candidates[i]].input[leadr_cnt] == 0) {
                terminal_candidate_exists = true;
                terminal_candidate_index = leadr_candidates[i];
            }
        }
        leadr_cnt++;
        if (leadr_cnt == LEADR_INPUT_MAXLEN && j > 1) {
            j = 1;  // probably a duplicate entry in the table, take the first
        }
        leadr_candidate_cnt = j;
        switch (j) {
            case 0:
                if (terminal_candidate_exists) {
                    send_sequence(terminal_candidate_index);
                    return leadr_end(keycode, LT_CONT);
                }
                return leadr_end(keycode, LT_INVL);
            case 1:
#ifndef LEADR_AUTOCOMPLETE_UNIQ
                if (leadr_cnt == LEADR_INPUT_MAXLEN 
                    || leadr_table[leadr_candidates[0]].input[leadr_cnt] == 0) {
                    send_sequence(leadr_candidates[0]);
                    return leadr_end(keycode, LT_NORM);
                }
                return true;
#else
                send_sequence(leadr_candidates[0]);
                return leadr_end(keycode, LT_UNIQ);
#endif
        }
        return true;
    }
    return false;
}
