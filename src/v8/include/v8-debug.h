// Copyright 2008 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_DEBUG_H_
#define V8_V8_DEBUG_H_

#include "v8.h"  // NOLINT(build/include)

/**
 * Debugger support for the V8 JavaScript engine.
 */
namespace v8 {

// Debug events which can occur in the V8 JavaScript engine.
enum DebugEvent {
  Break = 1,
  Exception = 2,
  NewFunction = 3,
  BeforeCompile = 4,
  AfterCompile = 5,
  CompileError = 6,
  AsyncTaskEvent = 7,
};

class V8_EXPORT Debug {
 public:
  /**
   * A client object passed to the v8 debugger whose ownership will be taken by
   * it. v8 is always responsible for deleting the object.
   */
  class ClientData {
   public:
    virtual ~ClientData() {}
  };


  /**
   * A message object passed to the debug message handler.
   */
  class Message {
   public:
    /**
     * Check type of message.
     */
    virtual bool IsEvent() const = 0;
    virtual bool IsResponse() const = 0;
    virtual DebugEvent GetEvent() const = 0;

    /**
     * Indicate whether this is a response to a continue command which will
     * start the VM running after this is processed.
     */
    virtual bool WillStartRunning() const = 0;

    /**
     * Access to execution state and event data. Don't store these cross
     * callbacks as their content becomes invalid. These objects are from the
     * debugger event that started the debug message loop.
     */
    virtual Local<Object> GetExecutionState() const = 0;
    virtual Local<Object> GetEventData() const = 0;

    /**
     * Get the debugger protocol JSON.
     */
    virtual Local<String> GetJSON() const = 0;

    /**
     * Get the context active when the debug event happened. Note this is not
     * the current active context as the JavaScript part of the debugger is
     * running in its own context which is entered at this point.
     */
    virtual Local<Context> GetEventContext() const = 0;

    /**
     * Client data passed with the corresponding request if any. This is the
     * client_data data value passed into Debug::SendCommand along with the
     * request that led to the message or NULL if the message is an event. The
     * debugger takes ownership of the data and will delete it even if there is
     * no message handler.
     */
    virtual ClientData* GetClientData() const = 0;

    virtual Isolate* GetIsolate() const = 0;

    virtual ~Message() {}
  };


  /**
   * An event details object passed to the debug event listener.
   */
  class EventDetails {
   public:
    /**
     * Event type.
     */
    virtual DebugEvent GetEvent() const = 0;

    /**
     * Access to execution state and event data of the debug event. Don't store
     * these cross callbacks as their content becomes invalid.
     */
    virtual Local<Object> GetExecutionState() const = 0;
    virtual Local<Object> GetEventData() const = 0;

    /**
     * Get the context active when the debug event happened. Note this is not
     * the current active context as the JavaScript part of the debugger is
     * running in its own context which is entered at this point.
     */
    virtual Local<Context> GetEventContext() const = 0;

    /**
     * Client data passed with the corresponding callback when it was
     * registered.
     */
    virtual Local<Value> GetCallbackData() const = 0;

    /**
     * Client data passed to DebugBreakForCommand function. The
     * debugger takes ownership of the data and will delete it even if
     * there is no message handler.
     */
    virtual ClientData* GetClientData() const = 0;

    virtual Isolate* GetIsolate() const = 0;

    virtual ~EventDetails() {}
  };

  /**
   * Debug event callback function.
   *
   * \param event_details object providing information about the debug event
   *
   * A EventCallback2 does not take possession of the event data,
   * and must not rely on the data persisting after the handler returns.
   */
  typedef void (*EventCallback)(const EventDetails& event_details);

  /**
   * Debug message callback function.
   *
   * \param message the debug message handler message object
   *
   * A MessageHandler2 does not take possession of the message data,
   * and must not rely on the data persisting after the handler returns.
   */
  typedef void (*MessageHandler)(const Message& message);

  /**
   * Callback function for the host to ensure debug messages are processed.
   */
  typedef void (*DebugMessageDispatchHandler)();

  static bool SetDebugEventListener(Isolate* isolate, EventCallback that,
                                    Local<Value> data = Local<Value>());

  // Schedule a debugger break to happen when JavaScript code is run
  // in the given isolate.
  static void DebugBreak(Isolate* isolate);

  // Remove scheduled debugger break in given isolate if it has not
  // happened yet.
  static void CancelDebugBreak(Isolate* isolate);

  // Check if a debugger break is scheduled in the given isolate.
  static bool CheckDebugBreak(Isolate* isolate);

  // Message based interface. The message protocol is JSON.
  static void SetMessageHandler(Isolate* isolate, MessageHandler handler);

  static void SendCommand(Isolate* isolate,
                          const uint16_t* command, int length,
                          ClientData* client_data = NULL);

 /**
  * Run a JavaScript function in the debugger.
  * \param fun the function to call
  * \param data passed as second argument to the function
  * With this call the debugger is entered and the function specified is called
  * with the execution state as the first argument. This makes it possible to
  * get access to information otherwise not available during normal JavaScript
  * execution e.g. details on stack frames. Receiver of the function call will
  * be the debugger context global object, however this is a subject to change.
  * The following example shows a JavaScript function which when passed to
  * v8::Debug::Call will return the current line of JavaScript execution.
  *
  * \code
  *   function frame_source_line(exec_state) {
  *     return exec_state.frame(0).sourceLine();
  *   }
  * \endcode
  */
  // TODO(dcarney): data arg should be a MaybeLocal
  static MaybeLocal<Value> Call(Local<Context> context,
                                v8::Local<v8::Function> fun,
                                Local<Value> data = Local<Value>());

  /**
   * Returns a mirror object for the given object.
   */
  static MaybeLocal<Value> GetMirror(Local<Context> context,
                                     v8::Local<v8::Value> obj);

  /**
   * Makes V8 process all pending debug messages.
   *
   * From V8 point of view all debug messages come asynchronously (e.g. from
   * remote debugger) but they all must be handled synchronously: V8 cannot
   * do 2 things at one time so normal script execution must be interrupted
   * for a while.
   *
   * Generally when message arrives V8 may be in one of 3 states:
   * 1. V8 is running script; V8 will automatically interrupt and process all
   * pending messages;
   * 2. V8 is suspended on debug breakpoint; in this state V8 is dedicated
   * to reading and processing debug messages;
   * 3. V8 is not running at all or has called some long-working C++ function;
   * by default it means that processing of all debug messages will be deferred
   * until V8 gets control again; however, embedding application may improve
   * this by manually calling this method.
   *
   * Technically this method in many senses is equivalent to executing empty
   * script:
   * 1. It does nothing except for processing all pending debug messages.
   * 2. It should be invoked with the same precautions and from the same context
   * as V8 script would be invoked from, because:
   *   a. with "evaluate" command it can do whatever normal script can do,
   *   including all native calls;
   *   b. no other thread should call V8 while this method is running
   *   (v8::Locker may be used here).
   *
   * "Evaluate" debug command behavior currently is not specified in scope
   * of this method.
   */
  static void ProcessDebugMessages(Isolate* isolate);

  /**
   * Debugger is running in its own context which is entered while debugger
   * messages are being dispatched. This is an explicit getter for this
   * debugger context. Note that the content of the debugger context is subject
   * to change. The Context exists only when the debugger is active, i.e. at
   * least one DebugEventListener or MessageHandler is set.
   */
  static Local<Context> GetDebugContext(Isolate* isolate);

  /**
   * While in the debug context, this method returns the top-most non-debug
   * context, if it exists.
   */
  static MaybeLocal<Context> GetDebuggedContext(Isolate* isolate);

  /**
   * Enable/disable LiveEdit functionality for the given Isolate
   * (default Isolate if not provided). V8 will abort if LiveEdit is
   * unexpectedly used. LiveEdit is enabled by default.
   */
  static void SetLiveEditEnabled(Isolate* isolate, bool enable);

  /**
   * Returns array of internal properties specific to the value type. Result has
   * the following format: [<name>, <value>,...,<name>, <value>]. Result array
   * will be allocated in the current context.
   */
  static MaybeLocal<Array> GetInternalProperties(Isolate* isolate,
                                                 Local<Value> value);

  /**
   * Defines if the ES2015 tail call elimination feature is enabled or not.
   * The change of this flag triggers deoptimization of all functions that
   * contain calls at tail position.
   */
  static bool IsTailCallEliminationEnabled(Isolate* isolate);
  static void SetTailCallEliminationEnabled(Isolate* isolate, bool enabled);

  /**
   * Get/Set an internal flag that indicates whether the debugger is
   * connected to the DevToolsAgent.  The embedder should only call
   * HasDebuggerConnected() to check the connection state; the
   * DebuggerConnected() function is for internal use only.
   *
   * This can be useful for an embedder that wants to wait for the debugger
   * to be connected before executing scripts.
   */
  static void DebuggerConnected(bool connected);
  static bool HasDebuggerConnected();

  /**
   * By default, DevToolsAgent tells the debugger to continue when a
   * navigation event occurs.  The embedder can disable this to keep the
   * debugger in a paused state even when a navigation event occurs.
   */
  static void ContinueDebuggerOnNavigationEvent(bool enable);
  static bool ShouldContinueDebuggerOnNavigationEvent();

  /**
   * By default, DevToolsAgent tells the debugger to continue when the
   * inspected webview is closed.  The embedder can disable this to keep
   * the debugger in a paused state even after the inspected webview is
   * closed.
   */
  static void ContinueDebuggerOnWidgetClose(bool enable);
  static bool ShouldContinueDebuggerOnWidgetClose();
};


}  // namespace v8


#undef EXPORT


#endif  // V8_V8_DEBUG_H_
