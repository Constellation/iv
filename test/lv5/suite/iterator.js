function equal(lhs, rhs) {
  var i, len;
  if (lhs.length !== rhs.length) {
    return false;
  }

  for (i = 0, len = lhs.length; i < len; ++i) {
    if (lhs[i] !== rhs[i]) {
      return false;
    }
  }
  return true;
}
var i = "testing";
var res = [];
for (var k in i) {
  res.push(k, i[k]);
}
equal(['0', 't', '1', 'e', '2', 's', '3', 't', '4', 'i', '5', 'n', '6', 'g'], res);
