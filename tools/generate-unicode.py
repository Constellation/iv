#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import string
import re
import bisect

class Mapper(object):
  def __init__(self, keys, values, special):
    self.keys = keys
    self.values = values
    self.special_casing = special

  def map(self, ch):
    i = bisect.bisect_right(self.keys, ch) - 1
    if i >= 0:
      by2 = False
      start = self.keys[i]
      end = self.values[i * 2]
      mapping = self.values[i * 2 + 1]
      # print "val", ch, start, end, mapping
      if mapping == 0:
        # special casing
        if start == ch:
          return self.special_casing[end][2]
      else:
        if (start & 0x8000) != (end & 0x8000):
          end ^= 0x8000
          by2 = True
        if ch <= end:
          if by2 and ((ch & 1) != (start & 1)):
            return ch
          return (ch + mapping) % 0x10000
    return ch

  def dump_cache(self, f, t):
    result = []
    for ch in range(f, t):
      v = self.map(ch)
      if v > 0xFFFF:
        result.append(0)  # special casing
      else:
        result.append(v)
    return result

  def check(self, db, lower):
    for ch in range(0, 0xFFFF + 1):
      c = db.database[ch]
      target = -1
      if c is not None:
        if lower:
          target = c.lower()
        else:
          target = c.upper()
        if target is None:
          target = -1

      converted = self.map(ch)
      if target == -1:
        # print ch, converted, target
        assert converted == ch
      else:
        # print ch, converted, target
        assert converted == target


class RegExpGenerator(object):
  def __init__(self, detector):
    self.detector = detector

  def generate_identifier_start(self):
    r = [ ch for ch in range(0xFFFF + 1) if self.detector.is_identifier_start(ch)]
    return self._generate_range(r)

  def generate_identifier_part(self):
    r = [ ch for ch in range(0xFFFF + 1) if self.detector.is_identifier_part(ch)]
    return self._generate_range(r)

  def generate_non_ascii_identifier_start(self):
    r = [ ch for ch in xrange(0x0080, 0xFFFF + 1) if self.detector.is_identifier_start(ch)]
    return self._generate_range(r)

  def generate_non_ascii_identifier_part(self):
    r = [ ch for ch in range(0x0080, 0xFFFF + 1) if self.detector.is_identifier_part(ch)]
    return self._generate_range(r)

  def generate_non_ascii_separator_space(self):
    r = [ ch for ch in range(0x0080, 0xFFFF + 1) if self.detector.is_separator_space(ch)]
    return self._generate_range(r)

  def _generate_range(self, r):
    if len(r) == 0:
      return '[]'

    buf = []
    start = r[0]
    end = r[0]
    predict = start + 1
    r = r[1:]

    for code in r:
      if predict == code:
        end = code
        predict = code + 1
        continue
      else:
        if start == end:
          buf.append("\\u%04X" % start)
        elif end == start + 1:
          buf.append("\\u%04X\\u%04X" % (start, end))
        else:
          buf.append("\\u%04X-\\u%04X" % (start, end))
        start = code
        end = code
        predict = code + 1

    if start == end:
      buf.append("\\u%04X" % start)
    else:
      buf.append("\\u%04X-\\u%04X" % (start, end))

    return '[' + ''.join(buf) + ']'

class Character(object):
  def __init__(self, data):
    self.data = data
    self.__character = int(data[0], 16)
    self.__lower = None
    self.__upper = None
    self.__special_casing_lower = None
    self.__special_casing_lower_index = None
    self.__special_casing_upper = None
    self.__special_casing_upper_index = None

    v = self.data[13].strip()
    if len(v) != 0:
      self.__lower = int(v, 16)
      assert v != int(self.data[0], 16)

    v = self.data[12].strip()
    if len(v) != 0:
      self.__upper = int(v, 16)
      assert v != int(self.data[0], 16)

  def character(self):
    return self.__character

  def lower(self):
    if self.__special_casing_lower is not None:
      return self.__special_casing_lower
    return self.__lower

  def upper(self):
    if self.__special_casing_upper is not None:
      return self.__special_casing_upper
    return self.__upper

  def append_casing(self, case, index):
    # print "%04X %08X" % (self.character(), case[2])
    if case[1]:
      self.__special_casing_lower = case[2]
      self.__special_casing_lower_index = index
      self.__lower = None
    else:
      self.__special_casing_upper = case[2]
      self.__special_casing_upper_index = index
      self.__upper = None

  def have_lower(self):
    return (self.__lower is not None) or (self.__special_casing_lower is not None)

  def have_upper(self):
    return (self.__upper is not None) or (self.__special_casing_upper is not None)

  def special_casing_lower_index(self):
    return self.__special_casing_lower_index

  def special_casing_upper_index(self):
    return self.__special_casing_upper_index

class Detector(object):
  def __init__(self, source):
    data = []
    dictionary = {}
    character = []
    database = {}

    with open(source) as uni:
      flag = False
      first = 0
      for line in uni:
        line = line.strip()
        if len(line) == 0 or line[0] == '#':
          continue
        d = string.split(line, ";")
        val = int(d[0], 16)
        if flag:
          if re.compile("<.+, Last>").match(d[1]):
            # print "%s : u%X" % (d[1], val)
            flag = False
            for t in range(first, val+1):
              dictionary[t] = str(d[2])
              database[t] = Character(d)
          else:
            raise "Database Exception"
        else:
          if re.compile("<.+, First>").match(d[1]):
            # print "%s : u%X" % (d[1], val)
            flag = True
            first = val
          else:
            dictionary[val] = str(d[2])
            database[val] = Character(d)

    for i in range(0xFFFF + 1):
      if dictionary.get(i) == None:
        data.append("Un")
        character.append(None)
      else:
        data.append(dictionary[i])
        character.append(database[i])

    self.data = data
    self.database = character
    self.special_casing = []

  def merge_special_casing(self, source):
    data = []
    dictionary = {}
    character = []
    database = {}

    with open(source) as uni:
      flag = False
      first = 0
      for line in uni:
        line = line.strip()
        if len(line) == 0 or line[0] == '#':
          continue
        d = string.split(line, ";")
        val = int(d[0], 16)
        if flag:
          if re.compile("<.+, Last>").match(d[1]):
            # print "%s : u%X" % (d[1], val)
            flag = False
            for t in range(first, val+1):
              dictionary[t] = str(d[2])
              database[t] = d
          else:
            raise "Database Exception"
        else:
          if re.compile("<.+, First>").match(d[1]):
            # print "%s : u%X" % (d[1], val)
            flag = True
            first = val
          else:
            dictionary[val] = str(d[2])
            database[val] = d

    for k, data in database.items():
      # lower
      sp = data[1].strip().split(' ')
      if len(sp) == 2:
        self.special_casing.append((k, True, (int(sp[0], 16) << 16) | int(sp[1], 16)))
      elif len(sp[0]) != 0:
        self.database[k].__lower = int(sp[0], 16)

      # upper
      sp = data[3].strip().split(' ')
      if len(sp) == 2:
        self.special_casing.append((k, False, (int(sp[0], 16) << 16) | int(sp[1], 16)))
      elif len(sp[0]) != 0:
        self.database[k].__upper = int(sp[0], 16)

    for i, case in enumerate(self.special_casing):
      ch = self.database[case[0]]
      assert ch is not None
      ch.append_casing(case, i)

  def is_ascii(self, ch):
    return ch < 0x80

  def is_ascii_alpha(self, ch):
    v = ch | 0x20
    return v >= ord('a') and v <= ord('z')

  def is_decimal_digit(self, ch):
    return ch >= ord('0') and ch <= ord('9')

  def is_octal_digit(self, ch):
    return ch >= ord('0') and ch <= ord('7')

  def is_hex_digit(self, ch):
    v = ch | 0x20
    return self.is_decimal_digit(c) or (v >= ord('a') and v <= ord('f'))

  def is_digit(self, ch):
    return self.is_decimal_digit(ch) or self.data[ch] == 'Nd'

  def is_ascii_alphanumeric(self, ch):
    return self.is_decimal_digit(ch) or self.is_ascii_alpha(ch)

  def _is_non_ascii_identifier_start(self, ch):
    c = self.data[ch]
    return c == 'Lu' or c == 'Ll' or c == 'Lt' or c == 'Lm' or c == 'Lo' or c == 'Nl'

  def _is_non_ascii_identifier_part(self, ch):
    c = self.data[ch]
    return c == 'Lu' or c == 'Ll' or c == 'Lt' or c == 'Lm' or c == 'Lo' or c == 'Nl' or c == 'Mn' or c == 'Mc' or c == 'Nd' or c == 'Pc' or ch == 0x200C or ch == 0x200D

  def is_separator_space(self, ch):
    return self.data[ch] == 'Zs'

  def is_white_space(self, ch):
    return ch == ord(' ') or ch == ord("\t") or ch == 0xB or ch == 0xC or ch == 0x00A0 or ch == 0xFEFF or self.is_separator_space(ch)

  def is_line_terminator(self, ch):
    return ch == 0x000D or ch == 0x000A or self.is_line_or_paragraph_terminator(ch)

  def is_line_or_paragraph_terminator(self, ch):
    return ch == 0x2028 or ch == 0x2029

  def is_identifier_start(self, ch):
    if self.is_ascii(ch):
      return ch == ord('$') or ch == ord('_') or ch == ord('\\') or self.is_ascii_alpha(ch)
    return self._is_non_ascii_identifier_start(ch)

  def is_identifier_part(self, ch):
    if self.is_ascii(ch):
      return ch == ord('$') or ch == ord('_') or ch == ord('\\') or self.is_ascii_alphanumeric(ch)
    return self._is_non_ascii_identifier_part(ch)

class CategoryGenerator(object):
  def __init__(self, detector):
    self.detector = detector

def key_lookup_bind(list):
  SPECIAL_CASING = 0xFFFFFFFF
  keys = []
  values = []

  def output(begin, last, dis):
    if dis == 1:
      keys.append(begin[0])
      values.append(last[0])
      values.append(begin[2])
      # print "%04X - %04X" % (begin[0], begin[2])
      assert begin[2] != 0 and begin[2] <= 0xFFFF
    elif dis == 2:
      keys.append(begin[0])
      if (begin[0] & 0x8000) == (last[0] & 0x8000):
        v = last[0] ^ 0x8000
        assert (begin[0] & 0x8000) != v
        values.append(v)
        values.append(begin[2])
        # print "%04X - %04X" % (begin[0], begin[2])
        assert begin[2] != 0 and begin[2] <= 0xFFFF
    elif dis == SPECIAL_CASING:
      keys.append(begin[0])
      values.append(begin[1])
      values.append(0)
      # print "UNI %d" % (begin[0])
    else:
      keys.append(begin[0])
      values.append(last[0])
      values.append(begin[2])
      # print "%04X - %04X" % (begin[0], begin[2])
      assert begin[2] != 0 and begin[2] <= 0xFFFF

  distances = []
  for t in list:
    if t[1] > 0xFFFF:
      # special casing multi
      distances.append((t[0], t[2], SPECIAL_CASING + t[2], True))
    else:
      dis = t[1] - t[0]
      if dis < 0:
        # uint16_t overflow
        dis += 0x10000
      distances.append((t[0], t[1], dis, False))

  prev = None
  start = None
  distance = None
  for t in distances:
    if t[2] >= SPECIAL_CASING:
      # special casing
      assert t[3] == True
      if prev is not None:
        assert start is not None
        output(start, prev, distance)
      output(t, t, SPECIAL_CASING)
      prev = None
      start = None
      distance = None
      continue

    assert t[2] != 0

    if prev is None:
      prev = t
      start = t
      distance = None
      continue
    else:
      v = t[0] - prev[0]
      if distance is None:
        if v == 1:
          distance = 1
        elif v == 2:
          distance = 2
        else:
          output(start, prev, distance)
          prev = t
          start = t
          distance = None
          continue
      else:
        if v != distance:
          output(start, prev, distance)
          prev = t
          start = t
          distance = None
          continue

      if distance is not None:
        if prev[2] == t[2]:
          # passed
          prev = t
        else:
          # not passed
          output(start, prev, distance)
          prev = t
          start = t
          distance = None
          continue
      else:
        output(start, prev, distance)
        prev = t
        start = t
        disntace = None
        continue

  if prev is not None:
    output(start, prev, distance)

  return keys, values

def view8(l):
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 8 == 0:
      result.append(', '.join(current))
      current = []
    current.append("0x%04X" % (l[idx]))
  result.append(', '.join(current))
  print ',\n'.join(result)

def view4(l):
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 4 == 0:
      result.append(', '.join(current))
      current = []
    current.append("0x%08X" % (l[idx]))
  result.append(', '.join(current))
  print ',\n'.join(result)

def main(source, special):
  db = Detector(source)
  db.merge_special_casing(special)

  lower = []
  for ch in range(0x0000, 0xFFFF + 1):
    c = db.database[ch]
    if c is not None:
      if c.have_lower():
        lower.append(c)

  keys, values = key_lookup_bind(
      [(ch.character(), ch.lower(), ch.special_casing_lower_index()) for ch in lower])
  mapper = Mapper(keys, values, db.special_casing)
  print len(keys), len(values)
  # mapper.check(db, True)  # assertion
  # view8(keys)
  # view8(values)
  # print "0x%04X" % (mapper.map(0x7B))
  # view8([mapper.map(ch) for ch in range(0xc0, 1000)])
  view8(mapper.dump_cache(0xC0, 1000))

  upper = []
  for ch in range(0x0000, 0xFFFF + 1):
    c = db.database[ch]
    if c is not None:
      if c.have_upper():
        upper.append(c)

  keys, values = key_lookup_bind(
      [(ch.character(), ch.upper(), ch.special_casing_upper_index()) for ch in upper])
  mapper = Mapper(keys, values, db.special_casing)
  # print len(keys), len(values)
  # mapper.check(db, False)  # assertion
  # view8(keys)
  # view8(values)
  # view8([mapper.map(ch) for ch in range(0xb5, 1000)])

  # special casing
  # print len(db.special_casing)
  # view4([ ch[2] for ch in db.special_casing ])
  # view8(mapper.dump_cache(0xB5, 1000))


if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2])
