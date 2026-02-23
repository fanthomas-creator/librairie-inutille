#include "hr_adapter.h"
static hr_adapter_t* all[]={&hr_adapter_c,&hr_adapter_cpp,&hr_adapter_rust,&hr_adapter_zig,&hr_adapter_go,NULL};
hr_adapter_t* hr_adapter_detect(const char* p){
    for(int i=0;all[i];i++) if(all[i]->detect(p)) return all[i];
    return &hr_adapter_c;
}
hr_adapter_t* hr_adapter_for_lang(hr_lang_t lang){
    switch(lang){
        case HR_LANG_C:    return &hr_adapter_c;
        case HR_LANG_CPP:  return &hr_adapter_cpp;
        case HR_LANG_RUST: return &hr_adapter_rust;
        case HR_LANG_ZIG:  return &hr_adapter_zig;
        case HR_LANG_GO:   return &hr_adapter_go;
        default:           return NULL;
    }
}
