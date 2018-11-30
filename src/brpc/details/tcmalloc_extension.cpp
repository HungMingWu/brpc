#include <dlfcn.h>                               // dlsym
#include <stdlib.h>                              // getenv
#include <mutex>
#include "butil/compiler_specific.h"
#include "brpc/details/tcmalloc_extension.h"

namespace {
typedef MallocExtension* (*GetInstanceFn)();
static std::once_flag g_get_instance_fn_once;
static GetInstanceFn g_get_instance_fn = NULL;
static void InitGetInstanceFn() {
    g_get_instance_fn = (GetInstanceFn)dlsym(
        RTLD_NEXT, "_ZN15MallocExtension8instanceEv");
}
} // namespace

MallocExtension* BAIDU_WEAK MallocExtension::instance() {
    // On fedora 26, this weak function is NOT overriden by the one in tcmalloc
    // which is dynamically linked.The same issue can't be re-produced in
    // Ubuntu and the exact cause is unknown yet. Using dlsym to get the
    // function works around the issue right now. Note that we can't use dlsym
    // to fully replace the weak-function mechanism since our code are generally
    // not compiled with -rdynamic which writes symbols to the table that
    // dlsym reads.
    std::call_once(g_get_instance_fn_once, InitGetInstanceFn);
    if (g_get_instance_fn) {
        return g_get_instance_fn();
    }
    return NULL;
}

bool IsHeapProfilerEnabled() {
    return MallocExtension::instance() != NULL;
}

static bool check_TCMALLOC_SAMPLE_PARAMETER() {
    char* str = getenv("TCMALLOC_SAMPLE_PARAMETER");
    if (str == NULL) {
        return false;
    }
    char* endptr;
    int val = strtol(str, &endptr, 10);
    return (*endptr == '\0' && val > 0);
}

bool has_TCMALLOC_SAMPLE_PARAMETER() {
    static bool val = check_TCMALLOC_SAMPLE_PARAMETER();
    return val;
}
