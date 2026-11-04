// WebAssembly build: there is no native calling convention support.
// SimulIDE registers everything as asCALL_GENERIC there, so this is only
// reached if something slipped through; fail with a script exception.
#ifdef __EMSCRIPTEN__
#include "as_config.h"
#include "as_callfunc.h"
#include "as_context.h"

BEGIN_AS_NAMESPACE

asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *, void *, asDWORD *, void *, asQWORD &, void *)
{
    context->SetInternalException("Native calls are not supported in the browser build");
    return 0;
}

END_AS_NAMESPACE
#endif
