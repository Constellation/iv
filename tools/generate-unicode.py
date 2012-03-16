#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import string
import re
import bisect

UNASSIGNED = 1                   #  Cn
UPPERCASE_LETTER = 2             #  Lu
LOWERCASE_LETTER = 3             #  Ll
TITLECASE_LETTER = 4             #  Lt
CASED_LETTER = 5                 #  LC
MODIFIER_LETTER = 6              #  Lm
OTHER_LETTER = 7                 #  Lo
NON_SPACING_MARK = 8             #  Mn
ENCLOSING_MARK = 9               #  Me
COMBINING_SPACING_MARK = 10      #  Mc
DECIMAL_DIGIT_NUMBER = 11        #  Nd
LETTER_NUMBER = 12               #  Nl
OTHER_NUMBER = 13                #  No
SPACE_SEPARATOR = 14             #  Zs
LINE_SEPARATOR = 15              #  Zl
PARAGRAPH_SEPARATOR = 16         #  Zp
CONTROL = 17                     #  Cc
FORMAT = 18                      #  Cf
PRIVATE_USE = 19                 #  Co
SURROGATE = 20                   #  Cs
DASH_PUNCTUATION = 21            #  Pd
START_PUNCTUATION = 22           #  Ps
END_PUNCTUATION = 23             #  Pe
CONNECTOR_PUNCTUATION = 24       #  Pc
OTHER_PUNCTUATION = 25           #  Po
MATH_SYMBOL = 26                 #  Sm
CURRENCY_SYMBOL = 27             #  Sc
MODIFIER_SYMBOL = 28             #  Sk
OTHER_SYMBOL = 29                #  So
INITIAL_QUOTE_PUNCTUATION = 30   #  Pi
FINAL_QUOTE_PUNCTUATION = 31     #  Pf

Categories = {
  'Cn': UNASSIGNED,
  'Lu': UPPERCASE_LETTER,
  'Ll': LOWERCASE_LETTER,
  'Lt': TITLECASE_LETTER,
  'LC': CASED_LETTER,
  'Lm': MODIFIER_LETTER,
  'Lo': OTHER_LETTER,
  'Mn': NON_SPACING_MARK,
  'Me': ENCLOSING_MARK,
  'Mc': COMBINING_SPACING_MARK,
  'Nd': DECIMAL_DIGIT_NUMBER,
  'Nl': LETTER_NUMBER,
  'No': OTHER_NUMBER,
  'Zs': SPACE_SEPARATOR,
  'Zl': LINE_SEPARATOR,
  'Zp': PARAGRAPH_SEPARATOR,
  'Cc': CONTROL,
  'Cf': FORMAT,
  'Co': PRIVATE_USE,
  'Cs': SURROGATE,
  'Pd': DASH_PUNCTUATION,
  'Ps': START_PUNCTUATION,
  'Pe': END_PUNCTUATION,
  'Pc': CONNECTOR_PUNCTUATION,
  'Po': OTHER_PUNCTUATION,
  'Sm': MATH_SYMBOL,
  'Sc': CURRENCY_SYMBOL,
  'Sk': MODIFIER_SYMBOL,
  'So': OTHER_SYMBOL,
  'Pi': INITIAL_QUOTE_PUNCTUATION,
  'Pf': FINAL_QUOTE_PUNCTUATION
}

class CategoryMapper(object):
  def __init__(self, db):
    self.db = db
    self.cache = [Categories[self.db.get_category(ch)] for ch in range(0, 1000)]

    # first sequencial concat
    tmp = []
    start = None
    category = None
    for ch in range(1000, 0x10000):
      cat = Categories[self.db.get_category(ch)]
      if category is None:
        category = cat
        start = ch
      else:
        if category == cat:
          pass
        else:
          assert start is not None
          tmp.append((start, ch - 1, category))
          start = ch
          category = cat

    if start is not None:
      assert category is not None
      tmp.append((start, 0xFFFF, category))

    tmp2 = []
    latest = None
    prev = None
    start = None

    def output(start, prev, latest):
      v = 0
      if start[0] & 1:
        v = (start[2] << 8) | (prev[2])
      else:
        v = (prev[2] << 8) | (start[2])
      tmp2.append((start[0], latest[1], v, True))

    for t in tmp:
      if (t[0] - t[1]) != 0:
        # not one char tuple
        if start is not None:
          if prev is not None:
            assert latest is not None
            output(start, prev, latest)
            start = None
            prev = None
            latest = None
          else:
            tmp2.append((start[0], start[1], start[2], False))
            start = None
            latest = None
        else:
          assert prev is None
          assert latest is None
        tmp2.append((t[0], t[1], t[2], False))
        continue

      if start is None:
        start = t
        latest = t
        assert prev is None
        continue

      if prev is None:
        # start!
        prev = t
        latest = t
        continue

      assert start is not None and prev is not None

      if (t[0] & 1) == (start[0] & 1):
        if t[2] == start[2]:
          latest = t
          pass
        else:
          # break this
          output(start, prev, latest)
          prev = None
          latest = None
          start = t
      else:
        assert (t[0] & 1) == (prev[0] & 1)
        if t[2] == prev[2]:
          latest = t
          pass
        else:
          # break this
          output(start, prev, latest)
          prev = None
          latest = None
          start = t
    if start is not None:
      if prev is not None:
        assert latest is not None
        output(start, prev, latest)
      else:
        tmp2.append(start)
        tmp2.append((start[0], start[1], start[2], False))

    self.keys = []
    self.values = []
    for t in tmp2:
      self.keys.append(t[0])
      self.values.append(t[1])
      self.values.append(t[2])

  def map(self, ch):
    if ch < 1000:
      return self.cache[ch]
    i = bisect.bisect_right(self.keys, ch) - 1
    high = self.values[i * 2]
    if ch <= high:
      code = self.values[i * 2 + 1]
      if code < 0x100:
        return code
      if (ch & 1) == 1:
        return code >> 8
      else:
        return code & 0xFF
    return UNASSIGNED

  def check(self):
    for ch in range(0, 0x10000):
      c = Categories[self.db.get_category(ch)]
      target = self.map(ch)
      # print ch, c, target
      assert c == target

  def dump(self):
    view8(self.keys)
    view8(self.values)
    view16(self.cache)


class CaseMapper(object):
  def __init__(self, keys, values, special, case):
    self.keys = keys
    self.values = values
    self.special_casing = special
    self.case = case

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

  def check(self, db):
    for ch in range(0, 0xFFFF + 1):
      c = db.database[ch]
      target = -1
      if c is not None:
        if self.case:
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
        data.append("Cn")
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
        # print d
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
      elif len(sp) == 1:
        if len(sp[0]) != 0:
          self.database[k].__lower = int(sp[0], 16)
      elif len(sp) == 3:
        self.special_casing.append(
            (k, True,
              (int(sp[0], 16) << 32) | (int(sp[1], 16) << 16) | (int(sp[2], 16))))
      else:
        assert 0

      # upper
      sp = data[3].strip().split(' ')
      if len(sp) == 2:
        self.special_casing.append((k, False, (int(sp[0], 16) << 16) | int(sp[1], 16)))
      elif len(sp) == 1:
        if len(sp[0]) != 0:
          self.database[k].__upper = int(sp[0], 16)
      elif len(sp) == 3:
        self.special_casing.append(
            (k, False,
              (int(sp[0], 16) << 32) | (int(sp[1], 16) << 16) | (int(sp[2], 16))))
      else:
        assert 0

    for i, case in enumerate(self.special_casing):
      ch = self.database[case[0]]
      assert ch is not None
      # print case
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

  def get_category(self, ch):
    return self.data[ch]

def key_lookup_bind(list):
  SPECIAL_CASING = 0xFFFFFFFFFFFFFFFFFFFFFFFF
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

def view16(l):
  print len(l)
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 16 == 0:
      result.append(', '.join(current))
      current = []
    current.append("%2d" % (l[idx]))
  result.append(', '.join(current))
  print ',\n'.join(result)

def view8(l):
  print len(l)
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
  print len(l)
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 4 == 0:
      result.append(', '.join(current))
      current = []
    current.append("0x%08X" % (l[idx]))
  result.append(', '.join(current))
  print ',\n'.join(result)

def view3(l):
  print len(l)
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 3 == 0:
      result.append(', '.join(current))
      current = []
    current.append("0x%016X" % (l[idx]))
  result.append(', '.join(current))
  print ',\n'.join(result)

def view2(l):
  print len(l)
  result = []
  current = []
  for idx in range(0, len(l)):
    if idx != 0 and idx % 2 == 0:
      result.append(', '.join(current))
      current = []
    current.append("UINT64_C(0x%016X)" % (l[idx]))
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
  mapper = CaseMapper(keys, values, db.special_casing, True)
  mapper.check(db)  # assertion
  # view8(keys)
  # view8(values)
  # print "0x%04X" % (mapper.map(0x7B))
  # view8([mapper.map(ch) for ch in range(0xc0, 1000)])
  # view8(mapper.dump_cache(0xC0, 1000))

  upper = []
  for ch in range(0x0000, 0xFFFF + 1):
    c = db.database[ch]
    if c is not None:
      if c.have_upper():
        upper.append(c)

  keys, values = key_lookup_bind(
      [(ch.character(), ch.upper(), ch.special_casing_upper_index()) for ch in upper])
  mapper = CaseMapper(keys, values, db.special_casing, False)
  # print len(keys), len(values)
  mapper.check(db)  # assertion
#  view8(keys)
#  view8(values)
#  view8(mapper.dump_cache(0xb5, 1000))

  # special casing
  # view3([ ch[2] for ch in db.special_casing ])
  # view2([ ch[2] for ch in db.special_casing ])
  # view8(mapper.dump_cache(0xB5, 1000))
  #
  #
  #
  category = CategoryMapper(db)
  category.check()
  category.dump()

if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2])
