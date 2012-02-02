var data = [
  [0.0, 0.0],
  [-0.0, -0.0],
  [Infinity, Infinity],
  [-Infinity, -Infinity],
  [0.1, 0],
  [-0.1, 0],
  [1.0, 1],
  [-1.0, -1]
];

function test() {
  for (var i = 0, len = data.length; i < len; ++i) {
    var input = data[i][0], expected = data[i][1];
    if (Math.trunc(input) !== expected) {
      throw Error(input);
    }
  }
  return true;
}
test();
