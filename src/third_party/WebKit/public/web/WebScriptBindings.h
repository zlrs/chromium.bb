/*
 * Copyright (C) 2017 Bloomberg Finance L.P.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef WebScriptBindings_h
#define WebScriptBindings_h

#include "../platform/WebCommon.h"
#include "base/callback.h"

namespace v8 {
class Context;
template <class T> class Local;
}

namespace blink {

class WebString;

class WebScriptBindings {
public:
    // Creates a V8 context that can access the DOM.
    BLINK_EXPORT static v8::Local<v8::Context> createWebScriptContext();

    // Disposes of per-context data for a context created with 'createWebScriptContext()':
    BLINK_EXPORT static void disposeWebScriptContext(v8::Local<v8::Context> context);

    // Executes 'closure', synchronously, in a scope where script execution
    // is permitted:
    BLINK_EXPORT static void runUserAgentScript(base::Closure closure);
};

} // namespace blink

#endif

