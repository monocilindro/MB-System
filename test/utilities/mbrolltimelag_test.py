#!/usr/bin/env python3

# Copyright 2019 Google Inc. All Rights Reserved.
#
# See README file for copying and redistribution conditions.

"""Tests for mbrolltimelag command line app."""

import os
import subprocess
import unittest


class MbrolltimelagTest(unittest.TestCase):

  def setUp(self):
    self.cmd = '../../src/utilities/mbrolltimelag'

  def testNoArgs(self):
    cmd = [self.cmd]
    raised = False
    try:
      subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      raised = True
      self.assertEqual(2, e.returncode)
      self.assertIn(b'Unable to open data list file:', e.output)
      self.assertIn(b'datalist.mb-1', e.output)
      self.assertIn(b'Running mbnavlist', e.output)
      self.assertIn(b'0 roll data read from datalist.mb-1', e.output)
    self.assertTrue(raised)

  def testHelp(self):
    cmd = [self.cmd, '-h']
    output = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode()
    self.assertIn('Version', output)
    self.assertIn('extracts the roll time series', output)
    self.assertIn('usage:', output)
    self.assertIn('-Tnlag/lagmin/lagmax', output)

  def testHelpVerbose2(self):
    cmd = [self.cmd, '-h', '-V', '-V']
    output = subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode()
    self.assertIn('Version', output)
    self.assertIn('extracts the roll time series', output)
    self.assertIn('usage:', output)
    self.assertIn('-Tnlag/lagmin/lagmax', output)
    self.assertIn('dbg2', output)
    self.assertIn('kind:', output)

  # TODO(schwehr): Add tests of actual usage.


if __name__ == '__main__':
  unittest.main()
