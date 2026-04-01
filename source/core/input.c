#include "input.h"
#include "lang.h"

bool input_prompt_device_name(char *out, size_t max_len)
{
    SwkbdState swkbd;
    char buf[MAX_DEVICE_NAME] = {0};

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, (int)max_len - 1);
    swkbdSetHintText(&swkbd, lang_str(STR_DEVICE_NAME_HINT));
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);

    SwkbdButton button = swkbdInputText(&swkbd, buf, sizeof(buf));
    if (button == SWKBD_BUTTON_CONFIRM && buf[0] != '\0') {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

bool input_keyboard(const char *hint, const char *initial, char *out, size_t max_len)
{
    SwkbdState swkbd;
    char buf[256] = {0};

    if (initial && initial[0] != '\0')
        strncpy(buf, initial, sizeof(buf) - 1);

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, (int)max_len - 1);
    swkbdSetHintText(&swkbd, hint ? hint : "");
    swkbdSetInitialText(&swkbd, buf);
    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);

    SwkbdButton button = swkbdInputText(&swkbd, buf, sizeof(buf));
    if (button == SWKBD_BUTTON_CONFIRM) {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}
