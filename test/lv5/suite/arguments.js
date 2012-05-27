function test(a, a) {
  var ret = 0;
  ret |= arguments[0] !== 0;
  ret |= arguments[1] !== 1;
  a = 20;
  ret |= arguments[0] !== 0;
  ret |= arguments[1] !== 20;
  return ret;
}
!test(0, 1);
