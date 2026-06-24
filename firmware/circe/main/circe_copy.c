#include "circe_copy.h"

#include <stddef.h>

typedef struct {
    circe_pattern_key_t key;
    const char *name;
    const char *text;
} circe_copy_row_t;

static const circe_copy_row_t s_copy[] = {
    {CIRCE_PATTERN_GREET_FIRST_TODAY, "greet.first_today", "Welcome back."},
    {CIRCE_PATTERN_HOME_HEADING, "home.heading", "Welcome back."},
    {CIRCE_PATTERN_HOME_PROMPT, "home.prompt", "Ready to check in?"},
    {CIRCE_PATTERN_HOME_SUBLINE, "home.subline", "Start with your body."},
    {CIRCE_PATTERN_HOME_READY, "home.ready", "Ready"},
    {CIRCE_PATTERN_READY_PROMPT, "ready.prompt", "Ready to check in?"},
    {CIRCE_PATTERN_READY_BODY, "ready.body", "Start with your body."},
    {CIRCE_PATTERN_BODY_INVITE, "body.invite", "Would it help to start with your body?"},
    {CIRCE_PATTERN_BODY_UNKNOWN_OKAY, "body.unknown_okay", "Not knowing is okay. Start with what your body notices."},
    {CIRCE_PATTERN_BODY_AREA_PROMPT, "body.area_prompt", "What feels most noticeable? Tap a body area."},
    {CIRCE_PATTERN_BODY_SENSATION_PROMPT, "body.sensation_prompt", "What quality is there? Pick any that fit."},
    {CIRCE_PATTERN_BODY_INTENSITY_PROMPT, "body.intensity_prompt", "How strong is this right now?"},
    {CIRCE_PATTERN_COLOR_OPTIONAL_PROMPT, "color.optional_prompt", "If this moment had a color, pick one — or skip."},
    {CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE, "privacy.default_notice", "Saved privately on this device."},
    {CIRCE_PATTERN_SAVE_CONFIRMED, "save.confirmed", "Thank you for checking in."},
    {CIRCE_PATTERN_DELETE_CONFIRM, "delete.confirm", "Delete this entry permanently?"},
    {CIRCE_PATTERN_DELETE_DONE, "delete.done", "Deleted. It won't appear in your records."},
    {CIRCE_PATTERN_ERROR_STORAGE_MISSING, "error.storage_missing", "Storage isn't ready. Check the memory card."},
    {CIRCE_PATTERN_ERROR_SAVE_FAILED, "error.save_failed", "Couldn't save — try again?"},
    {CIRCE_PATTERN_QUICK_ONE_TAP, "quick.one_tap", "Take a quick note."},
    {CIRCE_PATTERN_QUICK_SAVED, "quick.saved", "Saved privately."},
    {CIRCE_PATTERN_QUICK_ADD_LATER, "quick.add_later", "You can add more later."},
    {CIRCE_PATTERN_HOME_BODY, "home.body", "Body"},
    {CIRCE_PATTERN_HOME_QUICK, "home.quick", "Quick"},
    {CIRCE_PATTERN_HOME_REVIEW, "home.review", "Review"},
    {CIRCE_PATTERN_REVIEW_EMPTY, "review.empty", "No entries yet."},
    {CIRCE_PATTERN_HOME_MORE, "home.more", "More"},
    {CIRCE_PATTERN_DIAG_TITLE, "diag.title", "Storage health"},
    {CIRCE_PATTERN_BODY_ADD_ANOTHER, "body.add_another", "Add another sensation?"},
    {CIRCE_PATTERN_BODY_CONTINUE, "body.continue_save", "Continue"},
    {CIRCE_PATTERN_NAV_BACK, "nav.back", "Back"},
    {CIRCE_PATTERN_NAV_CANCEL, "nav.cancel", "Cancel"},
    {CIRCE_PATTERN_PRIVACY_STANDALONE, "privacy.standalone",
     "CIRCE is standalone-first. Entries stay on this device unless you export them."},
    {CIRCE_PATTERN_STRAND_TODAY, "strand.today", "Today"},
    {CIRCE_PATTERN_EDIT_PROMPT, "edit.prompt", "What would you like to change?"},
    {CIRCE_PATTERN_EDIT_COLOR, "edit.color", "Change color"},
    {CIRCE_PATTERN_EDIT_ADD_SENSATION, "edit.add_sensation", "Add body sensation"},
    {CIRCE_PATTERN_EDIT_SAVED, "edit.saved", "Updated and saved privately."},
    {CIRCE_PATTERN_MORE_MENU, "more.menu", "Settings & themes"},
    {CIRCE_PATTERN_MORE_APPEARANCE, "more.appearance", "Appearance"},
    {CIRCE_PATTERN_MORE_STORAGE, "more.storage", "Storage"},
    {CIRCE_PATTERN_APPEARANCE_TITLE, "appearance.title", "Appearance"},
    {CIRCE_PATTERN_APPEARANCE_PROMPT, "appearance.prompt", "Choose a calm space."},
    {CIRCE_PATTERN_APPEARANCE_APPLIED, "appearance.applied", "Theme updated."},
    {CIRCE_PATTERN_CARD_READY_TITLE, "card.ready.title", "Ready"},
    {CIRCE_PATTERN_CARD_READY_DESC, "card.ready.desc", "Start with your body."},
    {CIRCE_PATTERN_CARD_QUICK_TITLE, "card.quick.title", "Quick"},
    {CIRCE_PATTERN_CARD_QUICK_DESC, "card.quick.desc", "1–3 minute reset"},
    {CIRCE_PATTERN_CARD_REVIEW_TITLE, "card.review.title", "Review"},
    {CIRCE_PATTERN_CARD_REVIEW_DESC, "card.review.desc", "Journal & trends"},
    {CIRCE_PATTERN_CARD_MORE_TITLE, "card.more.title", "More"},
    {CIRCE_PATTERN_CARD_MORE_DESC, "card.more.desc", "Settings & themes"},
    {CIRCE_PATTERN_RING_CALM, "ring.calm", "Calm"},
    {CIRCE_PATTERN_RING_FOCUS, "ring.focus", "Focus"},
    {CIRCE_PATTERN_RING_BALANCE, "ring.balance", "Balance"},
    {CIRCE_PATTERN_RING_ENERGY, "ring.energy", "Energy"},
};

const char *circe_copy_get(circe_pattern_key_t key)
{
    for (size_t i = 0; i < sizeof(s_copy) / sizeof(s_copy[0]); i++) {
        if (s_copy[i].key == key) {
            return s_copy[i].text;
        }
    }
    return "";
}

const char *circe_copy_key_name(circe_pattern_key_t key)
{
    for (size_t i = 0; i < sizeof(s_copy) / sizeof(s_copy[0]); i++) {
        if (s_copy[i].key == key) {
            return s_copy[i].name;
        }
    }
    return "unknown";
}
