#pragma once
#include <3ds.h>

typedef struct {
    uint32_t pressed; // frame press
    uint32_t held;    // frame held
} HIDFrame;

// a call should return non 0 to stop the frame loop
// > 0 to signal key map change and new keys will used next time, or a simple needed stop
// < 0 if an error occurred that it is necessary to stop the next keybinds
// both cases will make HIDMapper_RunFrame return earlier to the caller with a respective result
// failure to inform about a remap with > 0 will leads to an undefined behaviour.
typedef int (*HIDCall)(HIDFrame frame, void* arg);

// declare a HIDCall
#define HIDFUNC(x) int x(HIDFrame frame, void* arg)

typedef struct {
    uint32_t key; // used as direct bit mask, or set 0 to call always
    bool press;   // false to act on hold, true to act on press
    bool cancel;  // false if never to cancel, except error canceling or frame change.
    HIDCall func; // NULL will be ignored, if key is also 0, then it will end loop.
    void* arg;    // argument to pass to function call
} HIDBind;

// Will stop loops from going forward
// Stop point indicator
// It's just member values alone
#define HIDBINDNULL 0, false, true, NULL, NULL

// Return values for Binds
#define HIDBINDERROR -1
#define HIDBINDOK 0
#define HIDBINDCANCELFRAME 1
#define HIDBINDCHANGEDFRAME 2

// binding should point to an array of binds and end in NULLBIND as last element
// copy argument should be true for non constant arrays
// if not copied, any edits to the given array after this call should be done carefully.
// mapping is global
Result HIDMapper_SetMapping(const HIDBind* binding, bool copy);

// reset to no key binds
Result HIDMapper_ClearMapping();

// read HID and call functions in given order
Result HIDMapper_RunFrame();
