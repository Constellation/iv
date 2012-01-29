function test() {
  var inner = function(i, j) { return i + j + 10; };
  return inner(10, 10) + inner((inner = function() { return 20 })(), inner());
}
test() === 80;
