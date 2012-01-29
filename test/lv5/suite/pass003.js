function test1() {
  var x = 10;
  var y = 20;
  return x + ((y === 20) ? (x = 20) : x);
}

function test2() {
  var x = 10;
  var y = 20;
  return x + (y === 20) ? (x = 20) : x;
}
test1() === 30 && test2() == 20;
