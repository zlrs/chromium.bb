# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Cros Eventing library, for tracking tasks using a shared eventing system"""

from __future__ import print_function

from time import time
import json
import multiprocessing


# Static Keys and Strings
EVENT_ID = "id"

EVENT_START_TIME = "start_time"
EVENT_END_TIME = "end_time"

EVENT_STATUS = "status"
EVENT_STATUS_RUNNING = "running"
EVENT_STATUS_FINISHED = "finished"
EVENT_STATUS_FAIL = "failed"

EVENT_FAIL_MSG = "failure_message"

# Helper functions
def EventIdGenerator():
  """Returns multiprocess safe iterator that  generates locally unique id"""
  # NOTE: that is might be as issue if chromite libraries are imported.
  #       Update to chromite.lib.parallel.Manager().Value('i') if this
  #       becomes an issue
  eid = multiprocessing.Value('i')

  while True:
    with eid.get_lock():
      val = eid.value
      eid.value += 1
    yield val

# Global event id generator instance
_next_event_id = EventIdGenerator()


class Failure(Exception):
  """Exception to raise while running an event."""

  def __init__(self, message=None, status=EVENT_STATUS_FAIL):
    """Create event with an optional message, status can be overridden"""
    super(Failure, self).__init__()
    self.message = message
    self.status = status

  def __dict__(self):
    """Return dictionary"""
    d = {EVENT_STATUS: self.status}

    if self.message:
      d[EVENT_FAIL_MSG] = self.message

    return d

  def __repr__(self):
    """Return failure message"""
    return self.message if self.message else self.status


class Event(dict):
  """Stores metadata of an Event that is logged during a set of tasks"""

  def __init__(self, eid=None, data=None, emit_func=None):
    """Create an Event, and populate metadata

    Args:
    eid: (optional) unique id number for a given set of events
    data: (optional) metadata associated with the given Event
    emit_func: (optional) Callback function to be called when event is complete
            Function will be passed current event as sole argument
    """
    super(Event, self).__init__()

    self[EVENT_ID] = eid if eid is not None else _next_event_id.next()

    self[EVENT_START_TIME] = time()

    if data is not None:
      self.update(data)

    self.emit_func = emit_func

  def fail(self, message=None, status=EVENT_STATUS_FAIL):
    """Call to mark event as failed

    Args:
    message: optional message to be set in the event
    status: optional, override the default status 'failure'
    """
    self[EVENT_STATUS] = status

    if message is not None:
      self[EVENT_FAIL_MSG] = message

  def __enter__(self):
    """Called in Context Manager, does nothing"""
    self[EVENT_STATUS] = EVENT_STATUS_RUNNING
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    """Writes event to emit_func, if given"""
    self[EVENT_END_TIME] = time()

    if EVENT_STATUS not in self or self[EVENT_STATUS] == EVENT_STATUS_RUNNING:
      if exc_value == None:
        self[EVENT_STATUS] = EVENT_STATUS_FINISHED
      else:
        self[EVENT_STATUS] = EVENT_STATUS_FAIL
        self[EVENT_FAIL_MSG] = repr(exc_value)

      if isinstance(exc_value, Failure):
        # Don't propagate Failure up
        exc_value = None

    if self.emit_func is not None:
      self.emit_func(self)

    return True if exc_value == None else False


class EventLogger(object):
  """Logger to be generate, and emit multiple Events"""

  def __init__(self, emit_func, data=None):
    """Initialize EventLogger

    Args:
    emit_func: Function called with single argument, the Event that has occurred
    data: (optional) metadata that will be copied to all Events generate
    """

    self.emit_func = emit_func

    self.data = data if data else {}

    self.idGen = EventIdGenerator()

  def Event(self, data=None):
    """Returns new Event with with given Data"""
    d = self.data.copy()
    if data:
      d.update(data)
    return Event(eid=self.idGen.next(), data=d, emit_func=self.emit_func)


class EventFileLogger(EventLogger):
  """Event Logger that writes to a file"""

  def __init__(self, file_out, data=None, encoder_func=json.dumps):
    self.file_out = file_out
    self.encoder_func = encoder_func
    super(EventFileLogger, self).__init__(self.write_event, data=data)

  def write_event(self, event):
    """Writes Event(dict) to file"""
    self.file_out.write(self.encoder_func(event) + "\n")
