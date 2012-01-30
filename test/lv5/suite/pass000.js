function test() {
  var i = 20;
  function inner(i) {
    return i;
  }
  inner(i + inner(i = 30));
  return inner(i);
}
test() === 30;
