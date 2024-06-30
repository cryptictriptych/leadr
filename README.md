# Leadr

Leadr is a timeout-free QMK leader library. It aims to provide some
functionality that is either non-trivial or not currently available in the
standard leader library, while satisfying the needs of people who are both slow
and impatient (like myself). 

## Basic Setup

Add `SRC += leadr.c` to `rules.mk`. Add `#include "leadr.h"` to
`keymap.c`. Add the following lines to `process_record_user` in `keymap.c`, to
intercept keys with `leadr_process`:

```c
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        if (leadr_process(keycode)) {
            return false;
        }
    }
    ...
}
```

Any additional code in `process_record_user` should come after the
`leadr_process` call. Otherwise keystrokes might be intercepted/handled and
prematurely return. This will bypass `leadr_process` function and potentially
impact stability of leadr sequences. 

#### Exceptions for Advanced Operation

Careful placement of `leadr_process` call could be used to if you want to add
special handling for certain keys that shouldn't impact (or be included in)
leadr sequences, then those keys should be handled before `leadr_process` is
called. Such a use case might be a more general autocompletion library, where
the early return from the `if(leadr_process(...))` is bypassed, allowing 
characters to be output during a leadr sequence, and sending only the
remaining characters upon a match. Combining this idea with some of the other
features described below would allow this feature to be run seamlessly
alongside standard leadr sequences using a small amount of user logic.

## Defining Leader Sequences

Define leadr sequences similar to QMK combos in a file called
`leadr_sequences.def` using the `LEAD` command:

```
LEAD("3.141592653589793116",  KC_P,KC_I)
LEAD("1.570796326794896558",  KC_P,KC_I,KC_2)
LEAD("0.785398163397448279",  KC_P,KC_I,KC_4)
LEAD("6.283185307179586232",  KC_2,KC_P,KC_I)
LEAD("2.718281828459045091",  KC_E)
LEAD("function",              KC_B,KC_F)
LEAD("func",                  KC_B,KC_F)
LEAD("git checkout",          KC_G,KC_C)
LEAD("git checkout -b",       KC_G,KC_C,KC_B)
LEAD(SS_LCTL(SS_TAP(X_B))";", RCTL_T(KC_SCLN))
```

The first element is the output, which takes the same format as `SEND_STRING`,
other keys are the sequence after the leadr key is pressed.

### Tagging Leadr Sequences for Custom Handling

Two variant commands are available for `leadr_sequences.def`. The first is
`LEADACT`, which can be used to define a label instead of a string output. This
label can be used to provide user-defined handling when a particular leadr
sequence is triggered. The other command is `LEADLBL`, which supports
user-defined handling _and_ an output string as would be defined with `LEAD`.

```
LEADACT(MY_CUSTOM_ACTION1, KC_G,KC_O)
LEADLBL(MY_CUSTOM_ACTION2, "current value: ",  KC_G,KC_O)
```

`LEADLBL` commands will be intercepted for user handling _after_ the string is
sent, but may be handy for performing additional actions such as playing
certain sounds etc.

## Defining a Leader Key

_Note: when defining multiple leadr keys, this step isn't required_

Mapping to an existing key can be done from `config.h`by defining `LEADR_KEY`,
e.g. `#define LEADR_KEY KC_RGUI`. If defining a custom leadr key, the
following line can be added to `keymap.c` once your key has been defined:

```
SET_LEADR_KEY(YOUR_LEADER_KEY_HERE)
``` 

## Basic Options

There is no hard limit on input sequence length, or number of output
characters, but the maximum values need to be specified in `config.h` if you
wish to override the default values (shown below). 

```
#define LEADR_INPUT_MAXLEN 6
#define LEADR_OUTPUT_MAXLEN 64
```

These can be adjusted to cut down on memory overhead or produce very long
sequences if desired. Note that _there is no implicit timeout_ on leadr
sequences. This is by design, as short timeouts become trickier to complete
successfully, and long timeouts incur significant delay on the output. This
variant of leadr sequences is for those of us that like a quick response, but
also occasionally need to pause and think for a moment.

If you wish to terminate a leadr sequence prematurely there are a few options.
The safest is to define a `LEADR_CANCEL_KEY` in `config.h`, such as

```
#define LEADR_CANCEL_KEY KC_ESC
```

This will override the ability to include the specified key (such as `KC_ESC`
as shown above) in a leadr sequence. However, invalid keys that don't match
any sequence will also kick out of leadr mode.

If you want to define longer sequences, but produce output as soon as you reach
a partial sequence where only one possible valid sequence remains, you can define

```
#define LEADR_AUTOCOMPLETE_UNIQ
```

This might be useful if you add a sequence, but intend to add more sequences
that might have overlapping characters, while not having to worry about
rewriting your original sequences. For instance, if you have a sequence to
write a file with `<LDR>w`, but think you might have other sequences starting
with `w`, you could specify the sequence as `<LDR>write`. If that's the only
sequence starting with `w`, it will complete once you hit `w`. If you add
another sequence to edit your browser search field, and define it as
`<LDR>www`, then the `write` command will be sent as soon as you make it to
`wr` since no other sequences start with `wr`. Likewise, the `www` command will
complete after `ww`.

If you don't define `LEADR_AUTOCOMPLETE_UNIQ`, output will only be produced
after the full sequence is completed.

To repeat the last leadr sequence by double tapping the leadr key, add the
following define to your config.h:

```
#define DOUBLE_LEADR_REPEAT_LAST 
```


## Overlapping Sequences of Different Length

Timeouts are not used, meaning leadr sequences will wait forever to be
completed, explicitly canceled with `LEADR_CANCEL_KEY`, or reach an invalid
path, which will cancel the leadr sequence.

This can pose problems if you have two sequences where one is fully contained
in the other. For example, if you define `<LDR>pi` to print out the value of
Pi, and `<LDR>pi2` to print out the value of Pi/2, you'll end up in a blocking
state after typing `<LDR>pi` since there are still two possible branches at
this point. 

Rather than playing games with timing, such as the standard QMK leader library,
two different options exist.

1) Typing any other key that doesn't result in a valid sequence will send the
   sequence output as well as the key that falls outside the sequence. For
   instance, `<LDR>pi*` will produce `3.141592653589793116*`. The exception to
   this if the last key is the one defined for `LEADR_CANCEL_KEY`.
2) If you're at the end of a terminal sequence, such as `<LDR>pi`, you can hit
   the `<LDR>` key again to generate the output without any additional
   characters at the end. In other words `<LDR>pi<LDR>` will produce
   `3.141592653589793116`.

If you type a character that doesn't match a sequence, and you're not already
at a terminal sequence, no output is produced and the leadr sequence is
exited. For instance, `<LDR>p*` will produce nothing unless `<LDR>p` is
explicitly defined.


## Crossing Layers

Layer crossing is now handled, output keys are inspected but special layer
transer keys are filtered out.

Original:
> Layer crossing is not handled automatically, you'd have to work such changes
into your sequence. The exception is if you put your leadr key on a temporary
layer, in which case leaving the layer will have no effect (such as when you
use an `MO()` binding).


## User Hooks

Similar to standard leader functions, user hooks are exposed for start/end of sequence:

```
void leadr_start_user(void) {
...
}

void leadr_end_user(uint16_t keycode, enum leadr_end_type term_type) {
    switch (term_type) {
        case LT_CONT:
          PLAY_SONG(mysong);
          break;
        case LT_NORM:
          PLAY_SONG(vsong);
          break;
        case LT_ABRT:
          PLAY_SONG(gsong);
          break;
        case LT_INVL:
          PLAY_SONG(csong);
          break;
        default:
          break;
    }
}
```

User hooks don't currently support interception of the string output for a
sequence. However, information about the sequence termination is passed to the
user hook so you can define indicators to determine how the sequence failed or
succeeded, such as audio response as shown above.

Leadr sequence termination is encoded via the following enum:

```
enum leadr_end_type {
    LT_NORM,  // leadr sequence ended by reaching a unique entry
    LT_INVL,  // sequence aborted due to invalid sequence
    LT_ABRT,  // sequence aborted using LEADR_CANCEL_KEY
    LT_UNIQ,  // sequence completed early because remaining characters form a unique branch
    LT_FORC,  // multiple branches possible, but leadr key triggered current match
    LT_CONT   // multiple branches possible, but invalid continuation key forced current match 
};
```

`LT_UNIQ` will only trigger if `LEADR_AUTOCOMPLETE_UNIQ` is defined. `LT_ABRT`
will only trigger if `LEADR_CANCEL_KEY` is defined.

To check if a leadr sequence is active within QMK, the `leadr_active()`
function is provided (returns `bool`).


## Multiple Leadr Keys

If more than one leadr key is desired, multiple leadr keys can be used if 

```
#define MULTI_LEADR
```

is added to `config.h`. In this case, the leadr key must be included in the
key sequences defined in `leadr_sequences.def`. For instance, if you wanted to
define leadr keys `KLEAD1` and `KLEAD2`, for two versions of `<LDR>e`, you'd
write

```
LEAD("2.718281828459045091", KLEAD1, KC_E)
LEAD("exp()"SS_TAP(X_LEFT),  KLEAD2, KC_E)
```

If another sequence was defined, but not modified to include the leadr key in
the sequence, then the first key will be treated as a leadr key instead of its
standard function. For instance

```
LEAD("KC_EXEC", KC_RGUI, KC_E)
```

will intercept `KC_RGUI` as a leadr key instead of its normal function, so
beware of potential issues with clobbering keys that might be necessary to run
`QK_BOOT` if that poses problems for your ability to flash.

Note that if `DOUBLE_LEADR_REPEAT_LAST` is defined, then double tapping a leadr
key will repeat the last sequence used for that particular leadr key.

### Combining Leadrs

If using two (or more) leadrs, they can be combined in sequences. The
following setup is valid:

```
LEAD("2.718281828459045091", KLEAD1, KC_E)
LEAD("exp()"SS_TAP(X_LEFT),  KLEAD2, KC_E)
LEAD(C(KC_Q),                KLEAD1, KLEAD2, KC_E)
LEAD(C(A(KC_DEL)),           KLEAD2, KLEAD1, KC_E)
```

However, a leadr key cannot be used in it's own sequence since repeating the
initial leadr key will either commit a terminal sequence when multiple
branches are present, or cancel the sequence if it isn't a valid terminal
sequence.

## Leader Macros

In case you want more than 2 dynamic macros (up to 10), or want to dynamically bind macros to
different leadr sequences, macro option is provided. This is completely
independent of the standard QMK dynamic macro library.

### How to set it up

Define the number of macros (up to 10, but if you need more you could hack the
source code), and the output sequence max length (this is separate from
standard leadr output maxlen):

```c
#define NUM_MACROS 4
#define LEADR_MACRO_MAXLEN 128
```

Define a "leadr record" key, and tie it to the leadr library by adding the
following call to your keymap.c:

```c
SET_LEADR_RECORD_KEY(your_key_here)
```

### Recording a New Leadr Macro

1) Start a leadr sequence with your desired leadr key to be used to execute the
   macro, and follow it with your leadr macro record key.
2) Next, enter exactly 1 digit (0-9) for the target location in the macro
   table. If you specify `NUM_MACROS` as a value less than 10, then you will
   need to limit the numeric input at this step to be no larger than
   `NUM_MACROS - 1`.
3) Now, begin typing your _input_ leadr sequence (i.e. the key sequence after
   you hit the leadr key that will produce your macro output).
4) Hit your initial leadr key used in step (1) to complete the leadr _input_
   sequence.
5) Now begin typing your desired _output_ sequence that will be produced by the
   leadr key and input sequence you specified in steps (1) and (4). Note:
   during this step you will see keyboard output until you finish recording.
   You can also run other leadr sequences which start with a leadr key that
   isn't the same as the one being used for the current leadr macro. However,
   nested leadr sequences won't show up properly during recording. So just
   record keystrokes as if nested leadr sequences had fully played out.
6) When you're finished with the output sequence, hit the original leadr key
   used in step (1) to end the sequence.
7) Use your new leadr sequence to your heart's delight. Keep in mind that
   you'll have to re-record your leadr macro if your keyboard is power cycled
   or reprogrammed, so this is mainly for testing or quick and dirty macros
   that you don't need regularly.

If `DOUBLE_LEADR_REPEAT_LAST` is defined, then double tapping a leadr key might
not behave as expected if a leadr macro calls another leadr sequence. Instead
it will repeat the last leadr sequence within the macro. 

**Note that calling a leadr macro from another leadr macro is allowed, but if
you call mutually-recursive leadr sequences the keyboard might get stuck in an
infinite loop. Use with caution!**
