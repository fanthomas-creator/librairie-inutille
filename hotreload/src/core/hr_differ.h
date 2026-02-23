#ifndef HR_DIFFER_H
#define HR_DIFFER_H

typedef enum {
    HR_CHANGE_NONE = 0,
    HR_CHANGE_BODY_ONLY,
    HR_CHANGE_SIGNATURE,
    HR_CHANGE_STRUCT,
    HR_CHANGE_FULL
} hr_change_type_t;

hr_change_type_t hr_differ_analyze(const char* old_path, const char* new_path);

#endif
