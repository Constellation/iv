function toArrayCheck(str, len) {
  var ary = str.toArray();
  for (var i = 0; i < ary.length; ++i) {
    if (ary[i] !== str[i]) {
      return false;
    }
  }
  return ary.length === str.length && str.length === len;
}
toArrayCheck("TEST", 4) &&
toArrayCheck("日本語", 3) &&
toArrayCheck("日本+test語", 8) &&
toArrayCheck("A" + "B" + "C" + "D" + "E" + "F" + "G" + "H", 8);
