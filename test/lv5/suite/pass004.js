function test() {
  var inner = function() { return 10; };
  return inner() + inner(inner = function() { return 20; }, 1, 2, 3, 4, 5);
}
test() === 20;
