#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import string
import re
import bisect

class DB(object):
  PATTERN = re.compile('^(?P<key>.+)\s*:\s*(?P<value>.+)$')

  def __init__(self, source):
    self.__registry = []
    item = {}
    prev_key = None
    with open(source) as c:
      for line in c:
        line = line.strip()
        if line == '%%':
          # delimiter
          self.validate_and_append(item)
          item = {}
          prev_key = None
        else:
          m = self.PATTERN.match(line)
          if m:
            key = m.group('key')
            value = m.group('value')
            if item.has_key(key):
              prev = item[key]
              if isinstance(prev, list):
                item[key].append(value)
              else:
                item[key] = [item[key], value]
            else:
              item[key] = value
            prev_key = key
          else:
            if prev_key:
              v = item[prev_key]
              if isinstance(v, list):
                v[-1] = v[-1] + ' ' + line
              else:
                item[prev_key] = item[prev_key] + ' ' + line
      self.validate_and_append(item)

  def registry(self):
    return self.__registry

  def validate_and_append(self, item):
    if item.has_key('Type'):
      self.__registry.append(item)

def main(source):
  db = DB(source)


if __name__ == '__main__':
  main(sys.argv[1])
