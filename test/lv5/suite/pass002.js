function test() {
  var x = 10;
  return x + (x = 20) + x;
}
test() === 50;
