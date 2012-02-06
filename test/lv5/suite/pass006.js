function test() {
  var i = 20;
  return i + i++ + i;
}
test() === 61;
