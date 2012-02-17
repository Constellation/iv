function outer() {
  var a = 10, b = 20, c = 30, d = 40;
  function add(lhs, rhs) {
    return lhs + rhs;
  }
  a = add(a, b = 40);
  return a === 50 && b === 40 && c === 30 && d === 40;
}
outer();
