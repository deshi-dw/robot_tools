#include <stdio.h>
// #include <unistd.h>
#include <stdlib.h>
// #include <pthread.h>

#include "debug.h"
#include "inpt.h"

void on_button(int idx, int flags);
void on_value(int idx, int amount);

int main(int arc, char** argv) {
    printf("inpt version %s\n", inpt_version());

    debug.time.last_ms = 1;

    DEBUG_TIME_START("inpt_update");
    inpt_update();
    DEBUG_TIME_STOP();

    // pthread_t inpt_thread = {0};
    // pthread_create(&inpt_thread, NULL, (void*(*)(void*))inpt_start, NULL);

    // printf("device count: %i\n", inpt_hid_count());
    char** names = inpt_hid_list();
    puts("select a device:");
    for(int i = 0; i < inpt_hid_count(); i++) {
        printf("\t%i: %s", i, names[i]);
    }
    puts("\n");

    char cin[256];
    gets(cin);
    int selection = atoi(cin);

    if(inpt_hid_select(selection) < 0) {
        exit(-1);
    }

    inpt_hid_on_btn(on_button);
    inpt_hid_on_val(on_value);

    while(1) {
        DEBUG_TIME_START("inpt_update");
        inpt_update();
        DEBUG_TIME_STOP();
        if(debug_time_last_ms() > 100.0) {
            exit(0);
        }
        // puts("\n");
    }

    return 0;
}

void on_button(int idx, int flags) {
    printf("button[%i] set to %i\n", idx, flags);
}

void on_value(int idx, int amount) {
    printf("value[%i] set to %d\n", idx, amount);
}
