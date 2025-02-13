# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import webapp2

from google.appengine.api import taskqueue

from dashboard.pinpoint.models import change
from dashboard.pinpoint.models import job as job_module


class NewHandler(webapp2.RequestHandler):
  """Handler that cooks up a fresh Pinpoint job."""

  def post(self):
    # TODO(dtu): Read the parameters from the request object.
    # Not doing it for now because it's easier to run tests this way.
    configuration = 'linux'
    test_suite = 'tab_switching.typical_25'
    test = 'http://www.airbnb.com/'
    metric = 'asdf'
    commits = (('chromium/src', 'a'), ('chromium/src', 'b'))

    # Validate parameters.
    if metric and not test_suite:
      raise ValueError("Specified a metric but there's no test_suite to run.")

    # TODO: Validate commit hashes.

    # Convert parameters to canonical internal representation.

    # Create job.
    job = job_module.Job.New(
        configuration=configuration,
        test_suite=test_suite,
        test=test,
        metric=metric,
        auto_explore=True)

    # Add changes.
    for repository, git_hash in commits:
      base_commit = change.Dep(repository=repository, git_hash=git_hash)
      job.AddChange(change.Change(base_commit=base_commit))

    # Put job into datastore.
    job_id = job.put().urlsafe()

    # Start job.
    task = taskqueue.add(queue_name='job-queue', target='pinpoint',
                         url='/run/' + job_id)
    job.task = task.name
    job.put()

    # Show status page.
    # TODO: Should return a JSON result instead. Use Cloud Endpoints.
    self.redirect('/job/' + job_id)
